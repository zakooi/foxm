using System;
using Windows.Storage;

namespace FoxM.UwpHost.Services
{
    public enum UserAgentMode
    {
        MobileLumia,
        FirefoxAndroid,
        DesktopFirefox,
        DesktopChrome
    }

    /// <summary>
    /// Lưu trữ và điều khiển các cấu hình người dùng của FoxM Browser.
    /// </summary>
    public static class SettingsService
    {
        private static readonly ApplicationDataContainer Settings = ApplicationData.Current.LocalSettings;

        public static string SearchEngine
        {
            get => GetValue("SearchEngine", "DuckDuckGo");
            set => SetValue("SearchEngine", value);
        }

        public static string HomePageUrl
        {
            get => GetValue("HomePageUrl", "https://github.com/zakooi/foxm");
            set => SetValue("HomePageUrl", value);
        }

        public static bool AutoTrimMemory
        {
            get => GetValue("AutoTrimMemory", true);
            set => SetValue("AutoTrimMemory", value);
        }

        public static bool AmoledDarkMode
        {
            get => GetValue("AmoledDarkMode", true);
            set => SetValue("AmoledDarkMode", value);
        }

        public static UserAgentMode CurrentUserAgentMode
        {
            get => (UserAgentMode)GetValue("CurrentUserAgentMode", (int)UserAgentMode.FirefoxAndroid);
            set => SetValue("CurrentUserAgentMode", (int)value);
        }

        public static string GetSearchUrl(string query)
        {
            string encoded = Uri.EscapeDataString(query);
            switch (SearchEngine)
            {
                case "Google":
                    return $"https://www.google.com/search?q={encoded}";
                case "Bing":
                    return $"https://www.bing.com/search?q={encoded}";
                case "DuckDuckGo":
                default:
                    return $"https://duckduckgo.com/html/?q={encoded}";
            }
        }

        public static string GetUserAgentString(UserAgentMode mode)
        {
            switch (mode)
            {
                case UserAgentMode.MobileLumia:
                    return "Mozilla/5.0 (Mobile; Windows Phone 8.1; Android 4.0; ARM; Trident/7.0; Touch; rv:11.0; IEMobile/11.0) like iPhone OS 7_0_3 Mac OS X AppleWebKit/537 (KHTML, like Gecko) Mobile Safari/537";
                case UserAgentMode.DesktopFirefox:
                    return "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:128.0) Gecko/20100101 Firefox/128.0";
                case UserAgentMode.DesktopChrome:
                    return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36";
                case UserAgentMode.FirefoxAndroid:
                default:
                    return "Mozilla/5.0 (Android 14; Mobile; rv:128.0) Gecko/128.0 Firefox/128.0";
            }
        }

        private static T GetValue<T>(string key, T defaultValue)
        {
            if (Settings.Values.TryGetValue(key, out object val) && val is T typedVal)
            {
                return typedVal;
            }
            return defaultValue;
        }

        private static void SetValue<T>(string key, T value)
        {
            Settings.Values[key] = value;
        }
    }
}
