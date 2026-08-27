using System;

namespace FoxM.UwpHost.Services
{
    public enum ReaderTheme
    {
        Light,
        Sepia,
        Dark,
        AmoledBlack
    }

    /// <summary>
    /// Xử lý chế độ đọc báo/tin tức tối giản (Reader Mode) loại bỏ toàn bộ quảng cáo và bố cục rườm rà.
    /// </summary>
    public static class ReaderModeService
    {
        public static string GenerateReaderScript(ReaderTheme theme = ReaderTheme.AmoledBlack, int fontSizePt = 16)
        {
            string bgColor = "#000000";
            string textColor = "#E4E4E7";

            switch (theme)
            {
                case ReaderTheme.Light:
                    bgColor = "#FFFFFF";
                    textColor = "#18181B";
                    break;
                case ReaderTheme.Sepia:
                    bgColor = "#FBF0D9";
                    textColor = "#5F4B32";
                    break;
                case ReaderTheme.Dark:
                    bgColor = "#18181B";
                    textColor = "#E4E4E7";
                    break;
                case ReaderTheme.AmoledBlack:
                    bgColor = "#000000";
                    textColor = "#E4E4E7";
                    break;
            }

            return $@"
(function() {{
    try {{
        var article = document.querySelector('article') || document.querySelector('.post-content') || document.querySelector('.article-body') || document.body;
        var title = document.title || '';
        var content = article.innerHTML;

        var readerOverlay = document.getElementById('foxm-reader-container');
        if (!readerOverlay) {{
            readerOverlay = document.createElement('div');
            readerOverlay.id = 'foxm-reader-container';
            document.body.appendChild(readerOverlay);
        }}

        readerOverlay.style.position = 'fixed';
        readerOverlay.style.top = '0';
        readerOverlay.style.left = '0';
        readerOverlay.style.width = '100vw';
        readerOverlay.style.height = '100vh';
        readerOverlay.style.backgroundColor = '{bgColor}';
        readerOverlay.style.color = '{textColor}';
        readerOverlay.style.zIndex = '9999999';
        readerOverlay.style.overflowY = 'auto';
        readerOverlay.style.padding = '20px 16px 80px 16px';
        readerOverlay.style.boxSizing = 'border-box';
        readerOverlay.style.fontFamily = 'Segoe UI, -apple-system, sans-serif';
        readerOverlay.style.fontSize = '{fontSizePt}px';
        readerOverlay.style.lineHeight = '1.6';

        readerOverlay.innerHTML = '<div style=""max-width: 680px; margin: 0 auto;"">' +
            '<h1 style=""font-size: 1.4em; font-weight: bold; margin-bottom: 16px;"">' + title + '</h1>' +
            '<div style=""opacity: 0.9;"">' + content + '</div>' +
            '<div style=""margin-top: 40px; text-align: center;""><button onclick=""document.getElementById(\'foxm-reader-container\').remove()"" style=""padding: 8px 16px; background: #FFD85012; color: white; border: none; border-radius: 4px; font-weight: bold;"">Đóng Chế độ đọc</button></div>' +
            '</div>';
    }} catch(e) {{
        console.error('FoxM Reader Mode error:', e);
    }}
}})();
";
        }
    }
}
