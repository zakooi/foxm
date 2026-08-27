using System;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using FoxM.GoannaRuntime;
using FoxM.UwpHost.Services;

namespace FoxM.UwpHost
{
    public sealed partial class MainPage : Page
    {
        private const string DefaultHomePage = "https://duckduckgo.com";

        private TabManagerService _tabManager = new TabManagerService();
        private ObservableCollection<BookmarkItem> _bookmarks = new ObservableCollection<BookmarkItem>();
        private ObservableCollection<HistoryItem> _history = new ObservableCollection<HistoryItem>();
        private DispatcherTimer _memoryTimer;

        public MainPage()
        {
            this.InitializeComponent();

            _tabManager.ActiveTabChanged += TabManager_ActiveTabChanged;
            _tabManager.TabClosed += TabManager_TabClosed;

            _memoryTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(3)
            };
            _memoryTimer.Tick += MemoryTimer_Tick;
        }

        private async void Page_Loaded(object sender, RoutedEventArgs e)
        {
            _bookmarks = await BookmarkService.LoadBookmarksAsync();
            _history = await HistoryService.LoadHistoryAsync();

            BookmarksListView.ItemsSource = _bookmarks;
            HistoryListView.ItemsSource = _history;
            TabListView.ItemsSource = _tabManager.Tabs;

            _tabManager.CreateTab(DefaultHomePage, "DuckDuckGo");

            _memoryTimer.Start();
            UpdateMemoryDisplay();
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
            TabsButton.Content = $"\uD83D\uDDC2 {_tabManager.Tabs.Count}";
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
            WelcomeOverlay.Visibility = Visibility.Collapsed;
            ErrorOverlay.Visibility = Visibility.Collapsed;

            try
            {
                BrowserCore.Navigate(target);
            }
            catch (Exception ex)
            {
                ShowErrorState("Khong the mo URL: " + ex.Message);
            }
        }

        private void ShowErrorState(string message)
        {
            WebProgressBar.Visibility = Visibility.Collapsed;
            ErrorMessageText.Text = message;
            ErrorOverlay.Visibility = Visibility.Visible;
        }

        private void RetryButton_Click(object sender, RoutedEventArgs e)
        {
            ErrorOverlay.Visibility = Visibility.Collapsed;
            NavigateTo(UrlTextBox.Text);
        }

        private void QuickLink_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is string url)
            {
                NavigateTo(url);
            }
        }

        private void BrowserCore_NavigationStarting(string url)
        {
            WebProgressBar.Visibility = Visibility.Visible;
            WebProgressBar.IsIndeterminate = true;
            UrlTextBox.Text = url;
            if (_tabManager.ActiveTab != null)
            {
                _tabManager.ActiveTab.Url = url;
            }
            UpdateBookmarkStar(url);
        }

        private void BrowserCore_NavigationCompleted(string url, bool success)
        {
            WebProgressBar.Visibility = Visibility.Collapsed;
            BackButton.IsEnabled = BrowserCore.CanGoBack;
            ForwardButton.IsEnabled = BrowserCore.CanGoForward;

            if (!success)
            {
                ShowErrorState("Loi ket noi mang. Vui long kiem tra Wi-Fi / 3G.");
            }
            else
            {
                ErrorOverlay.Visibility = Visibility.Collapsed;
            }
        }

        private void BrowserCore_TitleChanged(string newTitle)
        {
            if (!string.IsNullOrWhiteSpace(newTitle) && _tabManager.ActiveTab != null)
            {
                _tabManager.ActiveTab.Title = newTitle;
                string url = BrowserCore.CurrentUrl;
                if (!string.IsNullOrWhiteSpace(url))
                {
                    HistoryService.AddEntryAsync(_history, newTitle, url);
                }
            }
        }

        private void BrowserCore_ProgressChanged(double progress)
        {
            if (progress > 0 && progress < 1.0)
            {
                WebProgressBar.Visibility = Visibility.Visible;
                WebProgressBar.IsIndeterminate = false;
                WebProgressBar.Value = progress * 100;
            }
            else if (progress >= 1.0)
            {
                WebProgressBar.Visibility = Visibility.Collapsed;
            }
        }

        private void UpdateBookmarkStar(string url)
        {
            if (_bookmarks == null) return;
            bool isBookmarked = BookmarkService.IsBookmarked(_bookmarks, url);
            BookmarkStarButton.Content = isBookmarked ? "\u2605" : "\u2B50";
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
            if (BrowserCore != null && BrowserCore.CanGoBack)
            {
                BrowserCore.GoBack();
            }
        }

        private void ForwardButton_Click(object sender, RoutedEventArgs e)
        {
            if (BrowserCore != null && BrowserCore.CanGoForward)
            {
                BrowserCore.GoForward();
            }
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            if (BrowserCore != null)
            {
                BrowserCore.Reload();
            }
            else
            {
                NavigateTo(UrlTextBox.Text);
            }
        }

        private void ReaderButton_Click(object sender, RoutedEventArgs e)
        {
            string readerScript = ReaderModeService.GenerateReaderScript(ReaderTheme.AmoledBlack, 16);
            BrowserCore.ExecuteScriptAsync(readerScript);
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
            _tabManager.CreateTab(DefaultHomePage, "DuckDuckGo");
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
                TabListView.SelectedItem = null;
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
            BrowserCore.MinimizeMemoryUsage();
            MemoryGuardService.ForceTrimMemory();
            UpdateMemoryDisplay();
        }
    }
}
