using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Windows.Data.Json;
using Windows.Storage;

namespace FoxM.UwpHost.Services
{
    public class HistoryItem
    {
        public string Id { get; set; } = Guid.NewGuid().ToString();
        public string Title { get; set; }
        public string Url { get; set; }
        public DateTime VisitedAt { get; set; } = DateTime.Now;

        public string DisplayTime => VisitedAt.ToString("HH:mm - dd/MM/yyyy");
    }

    /// <summary>
    /// Dịch vụ quản lý lịch sử duyệt web sử dụng Windows.Data.Json gốc cho Windows 10 Mobile.
    /// </summary>
    public static class HistoryService
    {
        private const string FileName = "foxm_history.json";
        private const int MaxHistoryCount = 500;

        public static async Task<ObservableCollection<HistoryItem>> LoadHistoryAsync()
        {
            try
            {
                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.GetFileAsync(FileName);
                string jsonString = await FileIO.ReadTextAsync(file);

                var list = new ObservableCollection<HistoryItem>();
                if (JsonArray.TryParse(jsonString, out JsonArray array))
                {
                    foreach (var itemVal in array)
                    {
                        var obj = itemVal.GetObject();
                        DateTime visited = DateTime.Now;
                        if (obj.ContainsKey("VisitedAt"))
                        {
                            DateTime.TryParse(obj.GetNamedString("VisitedAt"), out visited);
                        }

                        list.Add(new HistoryItem
                        {
                            Id = obj.ContainsKey("Id") ? obj.GetNamedString("Id") : Guid.NewGuid().ToString(),
                            Title = obj.ContainsKey("Title") ? obj.GetNamedString("Title") : "",
                            Url = obj.ContainsKey("Url") ? obj.GetNamedString("Url") : "",
                            VisitedAt = visited
                        });
                    }
                }
                return list;
            }
            catch
            {
                return new ObservableCollection<HistoryItem>();
            }
        }

        public static async Task AddEntryAsync(ObservableCollection<HistoryItem> history, string title, string url)
        {
            if (history == null || string.IsNullOrWhiteSpace(url) || url.StartsWith("about:", StringComparison.OrdinalIgnoreCase))
                return;

            string safeTitle = string.IsNullOrWhiteSpace(title) ? url : title;

            var existing = history.FirstOrDefault(h => h.Url.Equals(url, StringComparison.OrdinalIgnoreCase));
            if (existing != null)
            {
                history.Remove(existing);
            }

            history.Insert(0, new HistoryItem { Title = safeTitle, Url = url, VisitedAt = DateTime.Now });

            while (history.Count > MaxHistoryCount)
            {
                history.RemoveAt(history.Count - 1);
            }

            await SaveHistoryAsync(history);
        }

        public static async Task SaveHistoryAsync(IEnumerable<HistoryItem> history)
        {
            try
            {
                var array = new JsonArray();
                foreach (var h in history)
                {
                    var obj = new JsonObject();
                    obj.SetNamedValue("Id", JsonValue.CreateStringValue(h.Id ?? ""));
                    obj.SetNamedValue("Title", JsonValue.CreateStringValue(h.Title ?? ""));
                    obj.SetNamedValue("Url", JsonValue.CreateStringValue(h.Url ?? ""));
                    obj.SetNamedValue("VisitedAt", JsonValue.CreateStringValue(h.VisitedAt.ToString("o")));
                    array.Add(obj);
                }

                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.CreateFileAsync(FileName, CreationCollisionOption.ReplaceExisting);
                await FileIO.WriteTextAsync(file, array.Stringify());
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("[FoxM HistoryService] Error saving history: " + ex.Message);
            }
        }

        public static async Task ClearAllAsync(ObservableCollection<HistoryItem> history)
        {
            history?.Clear();
            await SaveHistoryAsync(new List<HistoryItem>());
        }
    }
}
