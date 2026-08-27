using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
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
    /// Dịch vụ quản lý lịch sử duyệt web an toàn, có tính năng tìm kiếm và dọn dẹp.
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
                string json = await FileIO.ReadTextAsync(file);
                var list = JsonSerializer.Deserialize<List<HistoryItem>>(json);
                return new ObservableCollection<HistoryItem>(list ?? new List<HistoryItem>());
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

            // Xóa mục trùng trước đó nếu có
            var existing = history.FirstOrDefault(h => h.Url.Equals(url, StringComparison.OrdinalIgnoreCase));
            if (existing != null)
            {
                history.Remove(existing);
            }

            history.Insert(0, new HistoryItem { Title = safeTitle, Url = url, VisitedAt = DateTime.Now });

            // Cắt giảm nếu vượt quá giới hạn để tiết kiệm RAM
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
                var folder = ApplicationData.Current.LocalFolder;
                var file = await folder.CreateFileAsync(FileName, CreationCollisionOption.ReplaceExisting);
                string json = JsonSerializer.Serialize(history.ToList(), new JsonSerializerOptions { WriteIndented = true });
                await FileIO.WriteTextAsync(file, json);
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
