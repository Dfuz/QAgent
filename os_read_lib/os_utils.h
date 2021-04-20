#ifndef OS_UTILS_H
#define OS_UTILS_H

#ifdef __linux
#include <linux/sysinfo.h>
#include <sys/sysinfo.h>

#include <proc/readproc.h>
#include <proc/sysinfo.h>

#include <sys/statfs.h>
#else
#error "curenntly unsupported"
#endif

//#include <utils.h>
#include <tuple>
#include <stdexcept>
#include <numeric>
#include <QMap>
#include <QObject>
#include <QFile>
#include <type_traits>
#include <QProcess>
#include <QTimer>
#include <QLocale>
#include <QDebug>

namespace OS_UTILS
{

/*!
 * \brief The PS_STRUCT struct - цпу и память в процентах по всем процессам
 */
struct PS_STRUCT
{
    QString user;
    QString command;
    uint cpu;
    long mem;
};

/*!
 * \brief The FS_STRUCT struct - тип память по всем файл системам
 */
struct FS_STRUCT
{
    QString type;
    ulong total;
    ulong free;
};

typedef QMap<QString, FS_STRUCT> FSMAP;
typedef QMap<uint, PS_STRUCT> PSMAP;

struct OS_STATUS
{
    size_t TotalFSSize;
    size_t FreeFSSize;

    ulong cpuLoad;
    ushort psCount;

    ulong MemoryTotal;
    ulong MemoryFree;

    FSMAP allFs;
    PSMAP allPs;
};

/*!
  \class OS_EVENTS
  \brief класс для вытягивания данных OS (пока только линукс)
 */
class OS_EVENTS : public QObject
{
    Q_OBJECT
public:
    enum OSCollectOptions
    {
        NO_FS = 1,          // 0b0001
        NO_MEM = 1 << 1,    // 0b0010
        NO_PS = 1 << 2      // 0b0100
    }; 

    explicit OS_EVENTS(QObject *parent = nullptr);

    void setTimer(int timer = 1000);

    void stopTimer() {evTimer.stop();};

    /*!
     * \brief pullOSStatus - Вытягивает статуи системы
     * \return Статус системы
     */
    static OS_STATUS pullOSStatus(int);
    OS_STATUS pullOSStatus();
    
    void setCollectOptions(int);

signals:
    void pulledOSStatus(OS_STATUS);

public slots:
    void pullOSStatusSlot();

private:
    QTimer evTimer{this};
    int collectOptions = 0b000;

    static PSMAP pullPSMAP();
    static FSMAP pullFSMAP();
    static uint countPcpu(const struct proc_t *);
};

}

QDebug operator<<(QDebug, const OS_UTILS::PS_STRUCT &);
QDebug operator<<(QDebug, const OS_UTILS::FS_STRUCT &);
QDebug operator<<(QDebug, const OS_UTILS::OS_STATUS &);

#endif // OS_UTILS_H
