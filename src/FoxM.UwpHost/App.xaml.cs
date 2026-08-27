using System;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using FoxM.UwpHost.Services;

namespace FoxM.UwpHost
{
    sealed partial class App : Application
    {
        public App()
        {
            this.InitializeComponent();
            this.Suspending += OnSuspending;
            this.Resuming += OnResuming;

            // Đăng ký giám sát bộ nhớ hệ thống Windows 10 Mobile
            MemoryManager.AppMemoryUsageLimitChanging += MemoryManager_AppMemoryUsageLimitChanging;
            MemoryManager.AppMemoryUsageIncreased += MemoryManager_AppMemoryUsageIncreased;
        }

        private void MemoryManager_AppMemoryUsageLimitChanging(object sender, AppMemoryUsageLimitChangingEventArgs e)
        {
            if (MemoryManager.AppMemoryUsageLevel == AppMemoryUsageLevel.High ||
                MemoryManager.AppMemoryUsageLevel == AppMemoryUsageLevel.OverLimit)
            {
                // Kích hoạt dọn rác SpiderMonkey & .NET GC
                MemoryGuardService.ForceTrimMemory();
            }
        }

        private void MemoryManager_AppMemoryUsageIncreased(object sender, object e)
        {
            if (MemoryManager.AppMemoryUsageLevel == AppMemoryUsageLevel.OverLimit)
            {
                MemoryGuardService.ForceTrimMemory();
            }
        }

        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
            Frame rootFrame = Window.Current.Content as Frame;

            if (rootFrame == null)
            {
                rootFrame = new Frame();
                rootFrame.NavigationFailed += OnNavigationFailed;
                Window.Current.Content = rootFrame;
            }

            if (e.PrelaunchActivated == false)
            {
                if (rootFrame.Content == null)
                {
                    rootFrame.Navigate(typeof(MainPage), e.Arguments);
                }
                Window.Current.Activate();
            }
        }

        void OnNavigationFailed(object sender, NavigationFailedEventArgs e)
        {
            throw new Exception("Failed to load Page " + e.SourcePageType.FullName);
        }

        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            try
            {
                // Giải phóng bộ nhớ trước khi vào trạng thái Suspend
                MemoryGuardService.ForceTrimMemory();
            }
            finally
            {
                deferral.Complete();
            }
        }

        private void OnResuming(object sender, object e)
        {
            System.Diagnostics.Debug.WriteLine("[FoxM Lifecycle] App Resumed.");
        }
    }
}
