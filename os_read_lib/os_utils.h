#ifndef OS_UTILS_H
#define OS_UTILS_H

#ifdef __linux
#include <linux/sysinfo.h>
#include <sys/sysinfo.h>

#include <proc/readproc.h>

#include <sys/statfs.h>
#else
#error "curenntly unsupported"
#endif

//#include <utils.h>
#include <tuple>
#include <stdexcept>
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
    explicit OS_EVENTS(QObject *parent = nullptr);

    void setTimer(int timer = 1000);

    void stopTimer() {evTimer.stop();};

    /*!
     * \brief pullOSStatus - Вытягивает статуи системы
     * \return Статус системы
     */
    static OS_STATUS pullOSStatus();

signals:
    void pulledOSStatus(OS_STATUS);

private:
    QTimer evTimer{this};
};

}

QDebug operator<<(QDebug, const OS_UTILS::PS_STRUCT &);
QDebug operator<<(QDebug, const OS_UTILS::FS_STRUCT &);
QDebug operator<<(QDebug, const OS_UTILS::OS_STATUS &);

#endif // OS_UTILS_H
