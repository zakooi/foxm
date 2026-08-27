#include "GoannaDOMParser.h"
#include <algorithm>
#include <cwctype>

using namespace FoxM::GoannaRuntime;

GoannaDOMParser::GoannaDOMParser() :
    m_totalContentHeight(0.0f),
    m_pageTitle(L"FoxM Browser")
{
}

std::wstring GoannaDOMParser::StripHtmlTags(const std::wstring& input)
{
    std::wstring result;
    bool inTag = false;
    for (wchar_t ch : input)
    {
        if (ch == L'<') inTag = true;
        else if (ch == L'>') inTag = false;
        else if (!inTag)
        {
            if (ch == L'\r' || ch == L'\n' || ch == L'\t') result += L' ';
            else result += ch;
        }
    }

    // Gộp khoảng trắng thừa
    std::wstring clean;
    bool lastSpace = false;
    for (wchar_t ch : result)
    {
        if (ch == L' ')
        {
            if (!lastSpace) clean += ch;
            lastSpace = true;
        }
        else
        {
            clean += ch;
            lastSpace = false;
        }
    }
    return clean;
}

void GoannaDOMParser::ParseAndLayout(const std::wstring& html, float viewportWidth)
{
    m_renderBoxes.clear();
    m_totalContentHeight = 20.0f;
    float effectiveWidth = viewportWidth > 40.0f ? viewportWidth - 32.0f : 400.0f;
    float currentY = 16.0f;

    // 1. Tìm tiêu đề trang <title>
    size_t titleStart = html.find(L"<title");
    if (titleStart != std::wstring::npos)
    {
        size_t titleClose = html.find(L">", titleStart);
        size_t titleEnd = html.find(L"</title>", titleClose);
        if (titleClose != std::wstring::npos && titleEnd != std::wstring::npos)
        {
            m_pageTitle = StripHtmlTags(html.substr(titleClose + 1, titleEnd - titleClose - 1));
        }
    }

    // 2. Phân tích các khối văn bản cơ bản
    size_t pos = 0;
    while (pos < html.length() && m_renderBoxes.size() < 200)
    {
        size_t tagOpen = html.find(L"<", pos);
        if (tagOpen == std::wstring::npos) break;

        size_t tagClose = html.find(L">", tagOpen);
        if (tagClose == std::wstring::npos) break;

        std::wstring tagHeader = html.substr(tagOpen + 1, tagClose - tagOpen - 1);
        std::transform(tagHeader.begin(), tagHeader.end(), tagHeader.begin(), ::towlower);

        // H1
        if (tagHeader.rfind(L"h1", 0) == 0)
        {
            size_t endH1 = html.find(L"</h1>", tagClose);
            if (endH1 != std::wstring::npos)
            {
                std::wstring text = StripHtmlTags(html.substr(tagClose + 1, endH1 - tagClose - 1));
                if (!text.empty())
                {
                    float boxH = 42.0f;
                    RenderBox box = { NodeType::Heading1, text, L"", 16.0f, currentY, effectiveWidth, boxH, 22.0f, true, D2D1::ColorF(D2D1::ColorF::OrangeRed), D2D1::ColorF(0, 0, 0, 0) };
                    m_renderBoxes.push_back(box);
                    currentY += boxH + 12.0f;
                }
                pos = endH1 + 5;
                continue;
            }
        }
        // H2
        else if (tagHeader.rfind(L"h2", 0) == 0)
        {
            size_t endH2 = html.find(L"</h2>", tagClose);
            if (endH2 != std::wstring::npos)
            {
                std::wstring text = StripHtmlTags(html.substr(tagClose + 1, endH2 - tagClose - 1));
                if (!text.empty())
                {
                    float boxH = 34.0f;
                    RenderBox box = { NodeType::Heading2, text, L"", 16.0f, currentY, effectiveWidth, boxH, 18.0f, true, D2D1::ColorF(D2D1::ColorF::White), D2D1::ColorF(0, 0, 0, 0) };
                    m_renderBoxes.push_back(box);
                    currentY += boxH + 10.0f;
                }
                pos = endH2 + 5;
                continue;
            }
        }
        // P (Paragraph)
        else if (tagHeader.rfind(L"p", 0) == 0)
        {
            size_t endP = html.find(L"</p>", tagClose);
            if (endP != std::wstring::npos)
            {
                std::wstring text = StripHtmlTags(html.substr(tagClose + 1, endP - tagClose - 1));
                if (!text.empty())
                {
                    float lineCount = static_cast<float>((text.length() / 35) + 1);
                    float boxH = lineCount * 22.0f + 8.0f;
                    RenderBox box = { NodeType::Paragraph, text, L"", 16.0f, currentY, effectiveWidth, boxH, 14.0f, false, D2D1::ColorF(D2D1::ColorF::LightGray), D2D1::ColorF(0, 0, 0, 0) };
                    m_renderBoxes.push_back(box);
                    currentY += boxH + 8.0f;
                }
                pos = endP + 4;
                continue;
            }
        }
        // HR (Divider)
        else if (tagHeader.rfind(L"hr", 0) == 0)
        {
            RenderBox box = { NodeType::Divider, L"", L"", 16.0f, currentY, effectiveWidth, 2.0f, 1.0f, false, D2D1::ColorF(D2D1::ColorF::DimGray), D2D1::ColorF(0, 0, 0, 0) };
            m_renderBoxes.push_back(box);
            currentY += 12.0f;
            pos = tagClose + 1;
            continue;
        }

        pos = tagClose + 1;
    }

    m_totalContentHeight = currentY + 40.0f;
}
