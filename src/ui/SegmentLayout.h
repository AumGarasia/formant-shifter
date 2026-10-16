#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace segmentlayout
{

struct Cell
{
    char glyph = ' ';      // a digit, '-' or ' '
    bool dot = false;      // decimal point after this cell
};

/** Lays `text` out in exactly `cellCount` seven-segment cells, right-aligned.
    - Digits, '-' and ' ' take a cell each; a '.' lights the decimal point of the previous cell.
    - Any other character is ignored, so a stray unit or suffix cannot corrupt the number.
    - If the text needs more cells than are available, every cell shows a dash: a visibly
      wrong reading is better than a silently truncated one. */
inline std::vector<Cell> layout (const std::string& text, std::size_t cellCount)
{
    std::vector<Cell> content;

    for (const char ch : text)
    {
        if (ch == '.')
        {
            if (! content.empty())
                content.back().dot = true;
        }
        else if ((ch >= '0' && ch <= '9') || ch == '-' || ch == ' ')
        {
            content.push_back ({ ch, false });
        }
    }

    if (content.size() > cellCount)
        return std::vector<Cell> (cellCount, Cell { '-', false });

    std::vector<Cell> result (cellCount - content.size());      // blank cells on the left
    result.insert (result.end(), content.begin(), content.end());
    return result;
}

} // namespace segmentlayout