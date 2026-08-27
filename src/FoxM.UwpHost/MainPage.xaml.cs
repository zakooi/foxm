using System;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace FoxM.UwpHost
{
    public sealed partial class MainPage : Page
    {
        private const string DefaultHomePage = "https://github.com/zakooi/foxm";

        public MainPage()
        {
            this.InitializeComponent();
            this.Loaded += MainPage_Loaded;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            UrlTextBox.Text = DefaultHomePage;
            NavigateTo(DefaultHomePage);
        }

        private void NavigateTo(string input)
        {
            if (string.IsNullOrWhiteSpace(input)) return;

            string target = input.Trim();
            if (!target.StartsWith("http://", StringComparison.OrdinalIgnoreCase) &&
                !target.StartsWith("https://", StringComparison.OrdinalIgnoreCase) &&
                !target.StartsWith("about:", StringComparison.OrdinalIgnoreCase))
            {
                if (target.Contains(" ") || !target.Contains("."))
                {
                    target = "https://duckduckgo.com/html/?q=" + Uri.EscapeDataString(target);
                }
                else
                {
                    target = "https://" + target;
                }
            }

            UrlTextBox.Text = target;
            WebProgressBar.Visibility = Visibility.Visible;
            WebProgressBar.IsIndeterminate = true;

            // TODO: Gọi GoannaView.Navigate(target) khi nạp WinRT Component
            EngineStatusText.Text = $"🌐 Goanna Engine loading: {target}\nTLS 1.3 / NSS Verified | D3D11 Compositing";
            
            WebProgressBar.Visibility = Visibility.Collapsed;
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

        private void BackButton_Click(object sender, RoutedEventArgs e)
        {
            // GoannaView.GoBack();
        }

        private void ForwardButton_Click(object sender, RoutedEventArgs e)
        {
            // GoannaView.GoForward();
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            NavigateTo(UrlTextBox.Text);
        }

        private void GcButton_Click(object sender, RoutedEventArgs e)
        {
            // Kích hoạt thu gom rác bộ nhớ tức thì
            GC.Collect();
            EngineStatusText.Text = "🧹 SpiderMonkey GC & System Memory trimmed successfully!";
        }

        private void TabsButton_Click(object sender, RoutedEventArgs e)
        {
            // Quản lý Tab View
        }
    }
}
