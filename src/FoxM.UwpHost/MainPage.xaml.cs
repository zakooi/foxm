using System;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using FoxM.UwpHost.Services;

namespace FoxM.UwpHost
{
    public sealed partial class MainPage : Page
    {
        private const string DefaultHomePage = "https://github.com/zakooi/foxm";

        private TabManagerService _tabManager = new TabManagerService();
        private ObservableCollection<BookmarkItem> _bookmarks = new ObservableCollection<BookmarkItem>();
        private ObservableCollection<HistoryItem> _history = new ObservableCollection<HistoryItem>();
        private DispatcherTimer _memoryTimer;

        public MainPage()
        {
            this.InitializeComponent();

            _tabManager.ActiveTabChanged += TabManager_ActiveTabChanged;
            _tabManager.TabClosed += TabManager_TabClosed;

            // Thiết lập đồng hồ đo RAM mỗi 3 giây
            _memoryTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(3)
            };
            _memoryTimer.Tick += MemoryTimer_Tick;
        }

        private async void Page_Loaded(object sender, RoutedEventArgs e)
        {
            // 1. Nối sự kiện từ GoannaEngineView C++/CX
            if (GoannaEngineView != null)
            {
                GoannaEngineView.NavigationStarting += GoannaEngineView_NavigationStarting;
                GoannaEngineView.NavigationCompleted += GoannaEngineView_NavigationCompleted;
                GoannaEngineView.ProgressChanged += GoannaEngineView_ProgressChanged;
                GoannaEngineView.TitleChanged += GoannaEngineView_TitleChanged;
            }

            // 2. Nạp Bookmarks & Lịch sử
            _bookmarks = await BookmarkService.LoadBookmarksAsync();
            _history = await HistoryService.LoadHistoryAsync();

            BookmarksListView.ItemsSource = _bookmarks;
            HistoryListView.ItemsSource = _history;
            TabListView.ItemsSource = _tabManager.Tabs;

            // 3. Mở Tab đầu tiên
            _tabManager.CreateTab(DefaultHomePage, "GitHub - FoxM");

            // 4. Khởi động giám sát RAM
            _memoryTimer.Start();
            UpdateMemoryDisplay();
        }

        private void GoannaEngineView_NavigationStarting(string url)
        {
            WebProgressBar.Visibility = Visibility.Visible;
            WebProgressBar.IsIndeterminate = false;
            WebProgressBar.Value = 10;
        }

        private void GoannaEngineView_ProgressChanged(double progress)
        {
            WebProgressBar.Value = progress * 100.0;
            if (progress >= 1.0)
            {
                WebProgressBar.Visibility = Visibility.Collapsed;
            }
        }

        private async void GoannaEngineView_TitleChanged(string newTitle)
        {
            if (_tabManager.ActiveTab != null && !string.IsNullOrWhiteSpace(newTitle))
            {
                _tabManager.ActiveTab.Title = newTitle;
                await HistoryService.AddEntryAsync(_history, newTitle, _tabManager.ActiveTab.Url);
            }
        }

        private void GoannaEngineView_NavigationCompleted(string url, bool success)
        {
            WebProgressBar.Visibility = Visibility.Collapsed;
            if (GoannaEngineView != null)
            {
                BackButton.IsEnabled = GoannaEngineView.CanGoBack;
                ForwardButton.IsEnabled = GoannaEngineView.CanGoForward;
            }
            UpdateBookmarkStar(url);
        }

        private void TabManager_ActiveTabChanged(object sender, BrowserTab tab)
        {
            if (tab == null) return;
            UrlTextBox.Text = tab.Url;
            NavigateTo(tab.Url, isTabSwitch: true);
            UpdateTabCount();
            UpdateBookmarkStar(tab.Url);
        }

        private void TabManager_TabClosed(object sender, BrowserTab e)
        {
            UpdateTabCount();
        }

        private void UpdateTabCount()
        {
            TabsButton.Content = $"🗂️ {_tabManager.Tabs.Count}";
        }

        private void NavigateTo(string input, bool isTabSwitch = false)
        {
            if (string.IsNullOrWhiteSpace(input)) return;

            string target = input.Trim();
            if (!target.StartsWith("http://", StringComparison.OrdinalIgnoreCase) &&
                !target.StartsWith("https://", StringComparison.OrdinalIgnoreCase) &&
                !target.StartsWith("about:", StringComparison.OrdinalIgnoreCase))
            {
                if (target.Contains(" ") || !target.Contains("."))
                {
                    target = SettingsService.GetSearchUrl(target);
                }
                else
                {
                    target = "https://" + target;
                }
            }

            UrlTextBox.Text = target;
            if (_tabManager.ActiveTab != null)
            {
                _tabManager.ActiveTab.Url = target;
            }

            UpdateBookmarkStar(target);

            // Điều hướng thực tế thông qua GoannaView C++/CX
            if (GoannaEngineView != null)
            {
                GoannaEngineView.Navigate(target);
            }
        }

        private void UpdateBookmarkStar(string url)
        {
            if (_bookmarks == null) return;
            bool isBookmarked = BookmarkService.IsBookmarked(_bookmarks, url);
            BookmarkStarButton.Content = isBookmarked ? "★" : "⭐";
        }

        private void MemoryTimer_Tick(object sender, object e)
        {
            UpdateMemoryDisplay();
            MemoryGuardService.CheckAndEnforceMemorySafety();
        }

        private void UpdateMemoryDisplay()
        {
            var report = MemoryGuardService.GetCurrentMemoryReport();
            RamUsageText.Text = $"{report.AppUsageMb} / {report.AppLimitMb} ({report.UsagePercentage:F0}%)";
            RamUsageBar.Value = Math.Min(100, report.UsagePercentage);
        }

        // =========================================================================
        // SỰ KIỆN GIAO DIỆN
        // =========================================================================

        private void GoButton_Click(object sender, RoutedEventArgs e)
        {
            NavigateTo(UrlTextBox.Text);
        }

        private void UrlTextBox_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == VirtualKey.Enter)
            {
                NavigateTo(UrlTextBox.Text);
                e.Handled = true;
            }
        }

        private void UrlTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            ClearUrlButton.Visibility = string.IsNullOrEmpty(UrlTextBox.Text) ? Visibility.Collapsed : Visibility.Visible;
        }

        private void ClearUrlButton_Click(object sender, RoutedEventArgs e)
        {
            UrlTextBox.Text = "";
            UrlTextBox.Focus(FocusState.Programmatic);
        }

        private async void BookmarkStarButton_Click(object sender, RoutedEventArgs e)
        {
            string url = UrlTextBox.Text;
            if (string.IsNullOrWhiteSpace(url) || _bookmarks == null) return;

            if (BookmarkService.IsBookmarked(_bookmarks, url))
            {
                var item = _bookmarks.FirstOrDefault(b => b.Url.Equals(url, StringComparison.OrdinalIgnoreCase));
                if (item != null) _bookmarks.Remove(item);
            }
            else
            {
                _bookmarks.Insert(0, new BookmarkItem { Title = _tabManager.ActiveTab?.Title ?? url, Url = url });
            }

            await BookmarkService.SaveBookmarksAsync(_bookmarks);
            UpdateBookmarkStar(url);
        }

        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            if (GoannaEngineView != null && GoannaEngineView.CanGoBack)
            {
                GoannaEngineView.GoBack();
                UrlTextBox.Text = GoannaEngineView.CurrentUrl;
            }
        }

        private void ForwardButton_Click(object sender, RoutedEventArgs e)
        {
            if (GoannaEngineView != null && GoannaEngineView.CanGoForward)
            {
                GoannaEngineView.GoForward();
                UrlTextBox.Text = GoannaEngineView.CurrentUrl;
            }
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            if (GoannaEngineView != null)
            {
                GoannaEngineView.Reload();
            }
            else
            {
                NavigateTo(UrlTextBox.Text);
            }
        }

        private void ReaderButton_Click(object sender, RoutedEventArgs e)
        {
            string readerScript = ReaderModeService.GenerateReaderScript(ReaderTheme.AmoledBlack, 16);
            if (GoannaEngineView != null)
            {
                GoannaEngineView.ExecuteScriptAsync(readerScript);
            }
        }

        private void TabsButton_Click(object sender, RoutedEventArgs e)
        {
            TabSwitcherOverlay.Visibility = Visibility.Visible;
        }

        private void MenuButton_Click(object sender, RoutedEventArgs e)
        {
            BookmarksOverlay.Visibility = Visibility.Visible;
            UpdateMemoryDisplay();
        }

        private void NewTab_Click(object sender, RoutedEventArgs e)
        {
            TabSwitcherOverlay.Visibility = Visibility.Collapsed;
            _tabManager.CreateTab(DefaultHomePage, "Tab Mới");
        }

        private void CloseTabSwitcher_Click(object sender, RoutedEventArgs e)
        {
            TabSwitcherOverlay.Visibility = Visibility.Collapsed;
        }

        private void CloseTab_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is BrowserTab tab)
            {
                _tabManager.CloseTab(tab);
            }
        }

        private void TabListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TabListView.SelectedItem is BrowserTab tab)
            {
                TabSwitcherOverlay.Visibility = Visibility.Collapsed;
                _tabManager.SwitchToTab(tab);
                TabListView.SelectedItem = null; // Reset selection để lần sau chạm cùng tab vẫn kích hoạt
            }
        }

        private void CloseBookmarks_Click(object sender, RoutedEventArgs e)
        {
            BookmarksOverlay.Visibility = Visibility.Collapsed;
        }

        private void BookmarkItem_Click(object sender, ItemClickEventArgs e)
        {
            if (e.ClickedItem is BookmarkItem b)
            {
                BookmarksOverlay.Visibility = Visibility.Collapsed;
                NavigateTo(b.Url);
            }
        }

        private void HistoryItem_Click(object sender, ItemClickEventArgs e)
        {
            if (e.ClickedItem is HistoryItem h)
            {
                BookmarksOverlay.Visibility = Visibility.Collapsed;
                NavigateTo(h.Url);
            }
        }

        private async void ClearHistory_Click(object sender, RoutedEventArgs e)
        {
            await HistoryService.ClearAllAsync(_history);
        }

        private void TrimMemoryNow_Click(object sender, RoutedEventArgs e)
        {
            if (GoannaEngineView != null)
            {
                GoannaEngineView.MinimizeMemoryUsage();
            }
            MemoryGuardService.ForceTrimMemory();
            UpdateMemoryDisplay();
        }
    }
}
