#include "os_utils.h"
#include <QDebug>

int main()
{
    qDebug()<<"{ALL}";
    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus(OS_UTILS::OS_EVENTS::All);
    qDebug()<<"{NO_PS | NO_FS}";
    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus(OS_UTILS::OS_EVENTS::NO_PS | OS_UTILS::OS_EVENTS::NO_FS);
    qDebug()<<"{NO_MEM}";
    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus(OS_UTILS::OS_EVENTS::NO_MEM);
    return 0;
}
