using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
using Windows.Storage;

namespace FoxM.UwpHost.Services
{
    public class BookmarkItem
    {
        public string Id { get; set; } = Guid.NewGuid().ToString();
        public string Title { get; set; }
        public string Url { get; set; }
        public string Favicon { get; set; } = "ms-appx:///Assets/Square44x44Logo.png";
        public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    }

    /// <summary>
    /// Quản lý danh sách Bookmark (Dấu trang) lưu bền vững trong LocalStorage.
    /// </summary>
    public static class BookmarkService
    {
        private const string FileName = "foxm_bookmarks.json";

        public static async Task<ObservableCollection<BookmarkItem>> LoadBookmarksAsync()
        {
            try
            {
                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.GetFileAsync(FileName);
                string json = await FileIO.ReadTextAsync(file);
                var list = JsonSerializer.Deserialize<List<BookmarkItem>>(json);
                return new ObservableCollection<BookmarkItem>(list ?? GetDefaultBookmarks());
            }
            catch
            {
                var defaults = GetDefaultBookmarks();
                await SaveBookmarksAsync(defaults);
                return defaults;
            }
        }

        public static async Task SaveBookmarksAsync(IEnumerable<BookmarkItem> bookmarks)
        {
            try
            {
                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.CreateFileAsync(FileName, CreationCollisionOption.ReplaceExisting);
                string json = JsonSerializer.Serialize(bookmarks.ToList(), new JsonSerializerOptions { WriteIndented = true });
                await FileIO.WriteTextAsync(file, json);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("[FoxM BookmarkService] Error saving bookmarks: " + ex.Message);
            }
        }

        public static bool IsBookmarked(IEnumerable<BookmarkItem> bookmarks, string url)
        {
            if (bookmarks == null || string.IsNullOrWhiteSpace(url)) return false;
            return bookmarks.Any(b => b.Url.TrimEnd('/').Equals(url.TrimEnd('/'), StringComparison.OrdinalIgnoreCase));
        }

        private static ObservableCollection<BookmarkItem> GetDefaultBookmarks()
        {
            return new ObservableCollection<BookmarkItem>
            {
                new BookmarkItem { Title = "GitHub - FoxM", Url = "https://github.com/zakooi/foxm" },
                new BookmarkItem { Title = "DuckDuckGo", Url = "https://duckduckgo.com/html/" },
                new BookmarkItem { Title = "Wikipedia", Url = "https://en.m.wikipedia.org" },
                new BookmarkItem { Title = "Hacker News", Url = "https://news.ycombinator.com" }
            };
        }
    }
}
