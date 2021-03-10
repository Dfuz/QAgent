#include "os_utils.h"
#include <QDebug>

int main()
{
    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus();
//    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus();
//    qDebug()<<OS_UTILS::OS_EVENTS::pullOSStatus();
    return 0;
}
