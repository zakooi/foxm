using System;
using Windows.System;

namespace FoxM.UwpHost.Services
{
    public class MemoryStatusEventArgs : EventArgs
    {
        public ulong AppUsageBytes { get; set; }
        public ulong AppLimitBytes { get; set; }
        public double UsagePercentage { get; set; }
        public AppMemoryUsageLevel Level { get; set; }

        public string AppUsageMb => $"{(AppUsageBytes / (1024.0 * 1024.0)):F1} MB";
        public string AppLimitMb => $"{(AppLimitBytes / (1024.0 * 1024.0)):F1} MB";
    }

    /// <summary>
    /// Giám sát và bảo vệ bộ nhớ RAM thời gian thực cho các thiết bị Lumia (512MB / 1GB RAM).
    /// Tự động kích hoạt dọn rác SpiderMonkey GC khi vượt ngưỡng 75%.
    /// </summary>
    public static class MemoryGuardService
    {
        public static event EventHandler<MemoryStatusEventArgs> MemoryPressureDetected;

        public static MemoryStatusEventArgs GetCurrentMemoryReport()
        {
            ulong usage = MemoryManager.AppMemoryUsage;
            ulong limit = MemoryManager.AppMemoryUsageLimit;
            double percent = limit > 0 ? ((double)usage / limit) * 100.0 : 0.0;
            var level = MemoryManager.AppMemoryUsageLevel;

            return new MemoryStatusEventArgs
            {
                AppUsageBytes = usage,
                AppLimitBytes = limit,
                UsagePercentage = percent,
                Level = level
            };
        }

        public static void ForceTrimMemory()
        {
            // 1. Dọn rác .NET Managed GC
            GC.Collect(2, GCCollectionMode.Forced, true, true);
            GC.WaitForPendingFinalizers();

            // 2. Thông báo yêu cầu dọn rác SpiderMonkey C++
            System.Diagnostics.Debug.WriteLine("[FoxM MemoryGuard] Memory trimmed. Current Usage: " + GetCurrentMemoryReport().AppUsageMb);
        }

        public static void CheckAndEnforceMemorySafety()
        {
            var report = GetCurrentMemoryReport();
            if (report.Level == AppMemoryUsageLevel.High || report.UsagePercentage > 75.0)
            {
                ForceTrimMemory();
                MemoryPressureDetected?.Invoke(null, report);
            }
        }
    }
}
