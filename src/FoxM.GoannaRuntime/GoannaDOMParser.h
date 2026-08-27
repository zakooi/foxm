#pragma once

#include <string>
#include <vector>
#include <d2d1_2.h>

namespace FoxM
{
    namespace GoannaRuntime
    {
        enum class NodeType
        {
            Heading1,
            Heading2,
            Heading3,
            Paragraph,
            Link,
            Divider,
            Container,
            ImagePlaceholder
        };

        struct RenderBox
        {
            NodeType type;
            std::wstring text;
            std::wstring href;
            float x;
            float y;
            float width;
            float height;
            float fontSize;
            bool isBold;
            D2D1_COLOR_F textColor;
            D2D1_COLOR_F bgColor;
        };

        /// <summary>
        /// Bộ phân tích cú pháp HTML và tính toán Layout Boxes cho Goanna Direct2D Compositor.
        /// </summary>
        class GoannaDOMParser
        {
        public:
            GoannaDOMParser();

            void ParseAndLayout(const std::wstring& html, float viewportWidth);
            const std::vector<RenderBox>& GetRenderBoxes() const { return m_renderBoxes; }
            float GetTotalContentHeight() const { return m_totalContentHeight; }
            std::wstring GetPageTitle() const { return m_pageTitle; }

        private:
            std::wstring ExtractTagContent(const std::wstring& source, const std::wstring& tag, size_t& pos);
            std::wstring StripHtmlTags(const std::wstring& input);

            std::vector<RenderBox> m_renderBoxes;
            float m_totalContentHeight;
            std::wstring m_pageTitle;
        };
    }
}
