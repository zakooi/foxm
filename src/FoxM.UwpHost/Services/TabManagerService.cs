using System;
using System.Collections.ObjectModel;
using System.Linq;

namespace FoxM.UwpHost.Services
{
    public class BrowserTab
    {
        public string Id { get; set; } = Guid.NewGuid().ToString();
        public string Title { get; set; } = "Tab Mới";
        public string Url { get; set; } = "about:blank";
        public string Favicon { get; set; } = "ms-appx:///Assets/Square44x44Logo.png";
        public bool IsActive { get; set; }
    }

    /// <summary>
    /// Quản lý danh sách các Tab đang mở, hỗ trợ đóng/mở/chuyển tab mượt mà.
    /// </summary>
    public class TabManagerService
    {
        public ObservableCollection<BrowserTab> Tabs { get; } = new ObservableCollection<BrowserTab>();
        public BrowserTab ActiveTab { get; private set; }

        public event EventHandler<BrowserTab> ActiveTabChanged;
        public event EventHandler<BrowserTab> TabClosed;

        public BrowserTab CreateTab(string url = "https://github.com/zakooi/foxm", string title = "FoxM")
        {
            var tab = new BrowserTab
            {
                Url = url,
                Title = title,
                IsActive = false
            };

            Tabs.Add(tab);
            SwitchToTab(tab);
            return tab;
        }

        public void SwitchToTab(BrowserTab tab)
        {
            if (tab == null || !Tabs.Contains(tab)) return;

            foreach (var t in Tabs)
            {
                t.IsActive = (t == tab);
            }

            ActiveTab = tab;
            ActiveTabChanged?.Invoke(this, tab);
        }

        public void CloseTab(BrowserTab tab)
        {
            if (tab == null || !Tabs.Contains(tab)) return;

            int index = Tabs.IndexOf(tab);
            Tabs.Remove(tab);
            TabClosed?.Invoke(this, tab);

            if (Tabs.Count == 0)
            {
                CreateTab();
            }
            else if (ActiveTab == tab)
            {
                int newIndex = Math.Max(0, index - 1);
                SwitchToTab(Tabs[newIndex]);
            }
        }
    }
}
