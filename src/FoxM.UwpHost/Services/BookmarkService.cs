using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Windows.Data.Json;
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
    /// Quản lý danh sách Bookmark (Dấu trang) sử dụng Windows.Data.Json gốc của Windows 10 Mobile.
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
                string jsonString = await FileIO.ReadTextAsync(file);

                var list = new ObservableCollection<BookmarkItem>();
                if (JsonArray.TryParse(jsonString, out JsonArray array))
                {
                    foreach (var itemVal in array)
                    {
                        var obj = itemVal.GetObject();
                        list.Add(new BookmarkItem
                        {
                            Id = obj.ContainsKey("Id") ? obj.GetNamedString("Id") : Guid.NewGuid().ToString(),
                            Title = obj.ContainsKey("Title") ? obj.GetNamedString("Title") : "",
                            Url = obj.ContainsKey("Url") ? obj.GetNamedString("Url") : "",
                            Favicon = obj.ContainsKey("Favicon") ? obj.GetNamedString("Favicon") : "ms-appx:///Assets/Square44x44Logo.png"
                        });
                    }
                }
                return list.Count > 0 ? list : GetDefaultBookmarks();
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
                var array = new JsonArray();
                foreach (var b in bookmarks)
                {
                    var obj = new JsonObject();
                    obj.SetNamedValue("Id", JsonValue.CreateStringValue(b.Id ?? ""));
                    obj.SetNamedValue("Title", JsonValue.CreateStringValue(b.Title ?? ""));
                    obj.SetNamedValue("Url", JsonValue.CreateStringValue(b.Url ?? ""));
                    obj.SetNamedValue("Favicon", JsonValue.CreateStringValue(b.Favicon ?? ""));
                    array.Add(obj);
                }

                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.CreateFileAsync(FileName, CreationCollisionOption.ReplaceExisting);
                await FileIO.WriteTextAsync(file, array.Stringify());
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
