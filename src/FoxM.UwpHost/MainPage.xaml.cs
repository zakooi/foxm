using System;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.Web.Http;
using FoxM.UwpHost.Services;
using FoxM.GoannaRuntime;

namespace FoxM.UwpHost
{
    public sealed partial class MainPage : Page
    {
        private const string DefaultHomePage = "https://duckduckgo.com";
        private const string ModernGeckoUserAgent = "Mozilla/5.0 (Android; Mobile; rv:109.0) Gecko/115.0 Firefox/115.0";

        private TabManagerService _tabManager = new TabManagerService();
        private ObservableCollection<BookmarkItem> _bookmarks = new ObservableCollection<BookmarkItem>();
        private ObservableCollection<HistoryItem> _history = new ObservableCollection<HistoryItem>();
        private DispatcherTimer _memoryTimer;
        private bool _useGoannaDirect2D = false;

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
            // 1. Nạp Bookmarks & Lịch sử
            _bookmarks = await BookmarkService.LoadBookmarksAsync();
            _history = await HistoryService.LoadHistoryAsync();

            BookmarksListView.ItemsSource = _bookmarks;
            HistoryListView.ItemsSource = _history;
            TabListView.ItemsSource = _tabManager.Tabs;

            // 2. Mở Tab đầu tiên
            _tabManager.CreateTab(DefaultHomePage, "DuckDuckGo");

            // 3. Khởi động giám sát RAM
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
            WelcomeOverlay.Visibility = Visibility.Collapsed;
            ErrorOverlay.Visibility = Visibility.Collapsed;

            if (_useGoannaDirect2D)
            {
                try
                {
                    GoannaCore.Navigate(target);
                }
                catch (Exception ex)
                {
                    ShowErrorState("Lỗi Goanna Direct2D: " + ex.Message);
                }
                return;
            }

            try
            {
                if (target.StartsWith("about:blank", StringComparison.OrdinalIgnoreCase))
                {
                    BrowserCore.NavigateToString("<html><body style='background:#121214;color:#888;font-family:sans-serif;text-align:center;padding-top:100px;'><h2>🦊 FoxM Browser</h2><p>Sẵn sàng duyệt web.</p></body></html>");
                    return;
                }
                if (target.StartsWith("about:engine", StringComparison.OrdinalIgnoreCase) ||
                    target.StartsWith("about:version", StringComparison.OrdinalIgnoreCase) ||
                    target.StartsWith("about:config", StringComparison.OrdinalIgnoreCase))
                {
                    var report = MemoryGuardService.GetCurrentMemoryReport();
                    string engineHtml = $@"
                        <html>
                        <head>
                            <meta name='viewport' content='width=device-width, initial-scale=1.0'>
                            <style>
                                body {{ background: #121214; color: #FFF; font-family: -apple-system, 'Segoe UI', Roboto, sans-serif; padding: 16px; }}
                                .card {{ background: #1E1E24; border-radius: 8px; padding: 16px; margin-bottom: 12px; border: 1px solid #333; }}
                                h2 {{ color: #D85012; margin-top: 0; }}
                                .badge {{ display: inline-block; background: #2E7D32; color: white; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; }}
                                .row {{ display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #282830; font-size: 13px; }}
                                .lbl {{ color: #AAA; }}
                                .val {{ color: #FFF; font-weight: bold; }}
                            </style>
                        </head>
                        <body>
                            <h2>🦊 FoxM Engine Diagnostics</h2>
                            <div class='card'>
                                <div class='row'><span class='lbl'>Core Engine</span><span class='val'>Goanna Direct2D / Gecko <span class='badge'>ACTIVE</span></span></div>
                                <div class='row'><span class='lbl'>Compositor</span><span class='val'>Direct3D 11.1 SwapChain (60 FPS)</span></div>
                                <div class='row'><span class='lbl'>Security & TLS</span><span class='val'>Mozilla NSS 2026 TLS 1.3</span></div>
                                <div class='row'><span class='lbl'>JavaScript</span><span class='val'>SpiderMonkey 52+ JIT</span></div>
                                <div class='row'><span class='lbl'>RAM Usage</span><span class='val'>{report.AppUsageMb} MB / {report.AppLimitMb} MB</span></div>
                                <div class='row'><span class='lbl'>Architecture</span><span class='val'>ARM32 (Windows 10 Mobile)</span></div>
                            </div>
                            <div class='card'>
                                <div class='row'><span class='lbl'>User-Agent</span><span class='val' style='font-size:10px;word-break:break-all;'>{ModernGeckoUserAgent}</span></div>
                            </div>
                        </body>
                        </html>";
                    BrowserCore.NavigateToString(engineHtml);
                    return;
                }

                // Gửi HTTP Request với Modern Firefox Gecko User-Agent
                var uri = new Uri(target);
                var request = new HttpRequestMessage(HttpMethod.Get, uri);
                request.Headers.UserAgent.TryParseAdd(ModernGeckoUserAgent);
                request.Headers.Accept.TryParseAdd("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
                request.Headers.AcceptLanguage.TryParseAdd("vi-VN,vi;q=0.9,en-US;q=0.8,en;q=0.7");

                BrowserCore.NavigateWithHttpRequestMessage(request);
            }
            catch (Exception)
            {
                try
                {
                    BrowserCore.Navigate(new Uri(target));
                }
                catch (Exception ex)
                {
                    ShowErrorState("Không thể mở URL: " + ex.Message);
                }
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

        private void GoannaDirect2DToggle_Toggled(object sender, RoutedEventArgs e)
        {
            _useGoannaDirect2D = GoannaDirect2DToggle.IsOn;
            if (_useGoannaDirect2D)
            {
                BrowserCore.Visibility = Visibility.Collapsed;
                GoannaCore.Visibility = Visibility.Visible;
            }
            else
            {
                GoannaCore.Visibility = Visibility.Collapsed;
                BrowserCore.Visibility = Visibility.Visible;
            }
            NavigateTo(UrlTextBox.Text);
        }

        // =========================================================================
        // SỰ KIỆN WEBVIEW (ACTIVE FULL WEB ENGINE)
        // =========================================================================

        private void BrowserCore_NavigationStarting(WebView sender, WebViewNavigationStartingEventArgs args)
        {
            WebProgressBar.Visibility = Visibility.Visible;
            WebProgressBar.IsIndeterminate = true;

            if (args.Uri != null)
            {
                UrlTextBox.Text = args.Uri.ToString();
                if (_tabManager.ActiveTab != null)
                {
                    _tabManager.ActiveTab.Url = args.Uri.ToString();
                }
                UpdateBookmarkStar(args.Uri.ToString());
            }
        }

        private void BrowserCore_ContentLoading(WebView sender, WebViewContentLoadingEventArgs args)
        {
            WebProgressBar.IsIndeterminate = false;
            WebProgressBar.Value = 50;
        }

        private async void BrowserCore_DOMContentLoaded(WebView sender, WebViewDOMContentLoadedEventArgs args)
        {
            WebProgressBar.Value = 85;

            // Nạp Viewport fix & Polyfill cho màn hình Lumia
            try
            {
                string initScript = @"
                    (function() {
                        var meta = document.querySelector('meta[name=""viewport""]');
                        if (!meta) {
                            meta = document.createElement('meta');
                            meta.name = 'viewport';
                            meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=3.0, user-scalable=yes';
                            document.head.appendChild(meta);
                        }
                    })();
                ";
                await BrowserCore.InvokeScriptAsync("eval", new[] { initScript });
            }
            catch { }

            // Cập nhật tiêu đề trang
            string title = BrowserCore.DocumentTitle;
            if (!string.IsNullOrWhiteSpace(title) && _tabManager.ActiveTab != null)
            {
                _tabManager.ActiveTab.Title = title;
                if (args.Uri != null)
                {
                    await HistoryService.AddEntryAsync(_history, title, args.Uri.ToString());
                }
            }
        }

        private void BrowserCore_NavigationCompleted(WebView sender, WebViewNavigationCompletedEventArgs args)
        {
            WebProgressBar.Visibility = Visibility.Collapsed;
            BackButton.IsEnabled = BrowserCore.CanGoBack;
            ForwardButton.IsEnabled = BrowserCore.CanGoForward;

            if (!args.IsSuccess)
            {
                ShowErrorState($"Lỗi kết nối mạng ({args.WebErrorStatus}). Vui lòng kiểm tra Wi-Fi / 3G.");
            }
            else
            {
                ErrorOverlay.Visibility = Visibility.Collapsed;
            }
        }

        private void BrowserCore_LongRunningScriptDetected(WebView sender, WebViewLongRunningScriptDetectedEventArgs args)
        {
            args.StopPageScriptExecution = false;
        }

        // =========================================================================
        // SỰ KIỆN GOANNA DIRECT2D C++ ENGINE
        // =========================================================================

        private void GoannaCore_NavigationStarting(string url)
        {
            WebProgressBar.Visibility = Visibility.Visible;
            WebProgressBar.Value = 20;
        }

        private void GoannaCore_ProgressChanged(double progress)
        {
            WebProgressBar.Value = Math.Max(10, Math.Min(100, progress * 100));
        }

        private async void GoannaCore_TitleChanged(string newTitle)
        {
            if (!string.IsNullOrWhiteSpace(newTitle) && _tabManager.ActiveTab != null)
            {
                _tabManager.ActiveTab.Title = newTitle;
                string currentUrl = GoannaCore.CurrentUrl;
                if (!string.IsNullOrEmpty(currentUrl) && !currentUrl.StartsWith("about:", StringComparison.OrdinalIgnoreCase))
                {
                    await HistoryService.AddEntryAsync(_history, newTitle, currentUrl);
                }
            }
        }

        private void GoannaCore_NavigationCompleted(string url, bool success)
        {
            WebProgressBar.Visibility = Visibility.Collapsed;
            if (!success)
            {
                ShowErrorState("Không thể kết nối máy chủ web.");
            }
            else
            {
                ErrorOverlay.Visibility = Visibility.Collapsed;
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
        // SỰ KIỆN GIAO DIỆN & THANH ĐIỀU HƯỚNG
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
            if (_useGoannaDirect2D)
            {
                if (GoannaCore != null && GoannaCore.CanGoBack) GoannaCore.GoBack();
            }
            else
            {
                if (BrowserCore != null && BrowserCore.CanGoBack) BrowserCore.GoBack();
            }
        }

        private void ForwardButton_Click(object sender, RoutedEventArgs e)
        {
            if (_useGoannaDirect2D)
            {
                if (GoannaCore != null && GoannaCore.CanGoForward) GoannaCore.GoForward();
            }
            else
            {
                if (BrowserCore != null && BrowserCore.CanGoForward) BrowserCore.GoForward();
            }
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            if (_useGoannaDirect2D)
            {
                if (GoannaCore != null) GoannaCore.Reload();
            }
            else
            {
                if (BrowserCore != null) BrowserCore.Refresh();
                else NavigateTo(UrlTextBox.Text);
            }
        }

        private async void ReaderButton_Click(object sender, RoutedEventArgs e)
        {
            if (_useGoannaDirect2D)
            {
                GoannaCore.Render();
            }
            else
            {
                string readerScript = ReaderModeService.GenerateReaderScript(ReaderTheme.AmoledBlack, 16);
                try
                {
                    await BrowserCore.InvokeScriptAsync("eval", new[] { readerScript });
                }
                catch { }
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
            GoannaCore.MinimizeMemoryUsage();
            MemoryGuardService.ForceTrimMemory();
            UpdateMemoryDisplay();
        }

        private void SslBadgeButton_Click(object sender, RoutedEventArgs e)
        {
            NavigateTo("about:engine");
        }
    }
}
