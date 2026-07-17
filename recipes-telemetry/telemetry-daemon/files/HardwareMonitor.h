#pragma once

#include <QString>

class HardwareMonitor
{
public:
    static double getCpuTemperature();
    static double getLoadAverage();
    static int getRamUsagePercentage();
    static long getOsUptime();
    static bool getUndervoltageWarning();
};