#include "os_utils.h"
#include <qdebug.h>

void OS_UTILS::OS_EVENTS::setTimer(int timer)
{
    evTimer.callOnTimeout(this, &OS_UTILS::OS_EVENTS::pullOSStatusSlot);
    evTimer.start(timer);
}

OS_UTILS::OS_STATUS OS_UTILS::OS_EVENTS::pullOSStatus()
{
    struct sysinfo si;
    if (sysinfo(&si) == -1)
        throw std::runtime_error("failed to read sysinfo()");

    auto procs = readproctab(PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLCOM | PROC_FILLMEM);
    if (procs == NULL)
        throw std::runtime_error("failed to read procs");

    PSMAP allPs{};
    for(int i = 0; procs[i] != NULL; ++i)
        allPs.insert(procs[i]->tid, {
                         QString{procs[i]->ruser},
                         QString{procs[i]->cmd},
                         countPcpu(procs[i]),    //FIXME: count cpu by ->starttime/utime... mb...
                         procs[i]->size
                     });

    struct statfs fs;
    FSMAP allFs{};

    QFile mounts{"/proc/mounts"};
    if (!mounts.open(QFile::ReadOnly | QFile::Text))
        throw std::runtime_error("failed to read /proc/mounts");

    auto mounts_parsed = QString{mounts.readAll()}.split('\n', Qt::SkipEmptyParts);
    for(const auto &line : mounts_parsed) {
        auto parts = line.split(' ', Qt::SkipEmptyParts);
        if (!parts[1].startsWith('/')) continue;
        if (statfs(parts[1].toStdString().c_str(), &fs) == -1) continue;
        allFs.insert(parts[0], {
            parts[2],
            fs.f_blocks,
            fs.f_bfree
        });
    }

    size_t TotalFSSize = 0, FreeFSSize = 0;

    TotalFSSize = std::accumulate(std::next(allFs.cbegin()), allFs.cend(),
        allFs.first().total,
        [](const auto &total, const auto &fs) {return total + fs.total;});

    FreeFSSize = std::accumulate(std::next(allFs.cbegin()), allFs.cend(),
        allFs.first().free,
        [](const auto &free, const auto &fs) {return free + fs.free;});

    return {
        .TotalFSSize = TotalFSSize,
        .FreeFSSize = FreeFSSize,

        .cpuLoad = si.loads[0],
        .psCount = si.procs,
        .MemoryTotal = si.totalram,
        .MemoryFree = si.freeram,

        .allFs = allFs,
        .allPs = allPs
    };
}

// See https://github.com/credil/procps/blob/d51d065231ddaead393bc65185b0954040dbcbf2/ps/display.c#L308-L324 for example
uint OS_UTILS::OS_EVENTS::countPcpu(const struct proc_t * proc)
{
    uint retvar = 0;
    meminfo();
    auto seconds_since_boot = uptime(NULL, NULL);
    auto used_jiffies = proc->utime + proc->stime;
    auto avail_jiffies = seconds_since_boot * Hertz - proc->start_time;
    if (avail_jiffies) retvar = (used_jiffies << 24) / avail_jiffies;
    return retvar; 
}

void OS_UTILS::OS_EVENTS::pullOSStatusSlot()
{
    emit pulledOSStatus(pullOSStatus());
}

QDebug operator<<(QDebug debug, const OS_UTILS::OS_STATUS &stat)
{
    QDebugStateSaver saver(debug);
    debug.space()<<"Total(fs): "<<stat.TotalFSSize;
    debug.nospace()<<'\n';
    debug.space()<<"Free(fs): "<<stat.FreeFSSize;
    debug.nospace()<<'\n';

    debug.space()<<"Cpu load: "<<stat.cpuLoad;
    debug.nospace()<<'\n';
    debug.space()<<"Process count: "<<stat.psCount;
    debug.nospace()<<'\n';

    debug.space()<<"Total(ps): "<<stat.MemoryTotal;
    debug.nospace()<<'\n';
    debug.space()<<"Free(ps): "<<stat.MemoryFree;
    debug.nospace()<<'\n';

    debug.space()<<"FS Map: "<<stat.allFs;
    debug.nospace()<<'\n';
    debug.space()<<"PS Map: "<<stat.allPs;
    debug.nospace()<<'\n';
    return debug;
}

QDebug operator<<(QDebug debug, const OS_UTILS::FS_STRUCT &fs)
{
    QDebugStateSaver saver(debug);
    debug.space()<<"type: "<<fs.type;
    debug.space()<<"total: "<<fs.total;
    debug.space()<<"free: "<<fs.free;
    debug.nospace()<<'\n';
    return debug;
}

QDebug operator<<(QDebug debug, const OS_UTILS::PS_STRUCT &ps)
{
    QDebugStateSaver saver(debug);
    debug.space()<<"user: "<<ps.user;
    debug.space()<<"command: "<<ps.command;
    debug.space()<<"cpu: "<<ps.cpu;
    debug.space()<<"mem: "<<ps.mem;
    debug.nospace()<<'\n';
    return debug;
}
