#include "HardwareMonitor.h"
#include <QFile>
#include <QTextStream>
#include <sys/sysinfo.h>

double HardwareMonitor::getCpuTemperature()
{
    QFile file(QStringLiteral("/sys/class/thermal/thermal_zone0/temp"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString line = in.readLine();
        file.close();
        
        bool ok;
        double temp = line.toDouble(&ok);
        if (ok)
        {
            return temp / 1000.0;
        }
    }
    
    return 0.0;
}

double HardwareMonitor::getLoadAverage()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        // The kernel stores load averages as scaled fixed-point integers. 
        // We isolate the 1-minute load average (loads[0]) and divide by the bit-shifted scaling factor (1 << SI_LOAD_SHIFT).
        return static_cast<double>(info.loads[0]) / static_cast<double>(1 << SI_LOAD_SHIFT);
    }
    
    return 0.0;
}

int HardwareMonitor::getRamUsagePercentage()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        unsigned long total = info.totalram;
        unsigned long free = info.freeram;
        unsigned long used = total - free;
        return static_cast<int>((used * 100) / total);
    }
    
    return 0;
}

long HardwareMonitor::getOsUptime()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        return info.uptime;
    }
    
    return 0;
}

bool HardwareMonitor::getUndervoltageWarning()
{
    // The hardware monitor index assignment is dynamic depending on kernel module load order.
    // Iterate through hwmon0 to hwmon4 to locate the Raspberry Pi firmware voltage alarm flag.
    for (int i = 0; i < 5; ++i)
    {
        QString path = QStringLiteral("/sys/class/hwmon/hwmon%1/in0_lcrit_alarm").arg(i);
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            QString line = in.readLine();
            file.close();
            
            return (line.trimmed() == QStringLiteral("1"));
        }
    }
    
    return false;
}