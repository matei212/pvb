#pragma once

inline constexpr float SIDEBAR_WIDTH = 220.0f;
inline constexpr float SIDEBAR_PAD = 8.0f;
inline constexpr float SIDEBAR_ITEM_VSPACE = 12.0f;

inline constexpr float BLOCK_DRAG_THRESH = 5.0f;
inline constexpr ImU32 BLOCK_COLOR = IM_COL32(49, 96, 146, 255);
inline constexpr ImU32 BLOCK_TEXT_COLOR = IM_COL32(245, 248, 252, 255);
inline constexpr float BLOCK_HPAD = 14.0f;
inline constexpr float BLOCK_HSPACE = 6.0f;
inline constexpr float BLOCK_NOTCH_WIDTH = 28.0f;
inline constexpr float BLOCK_NOTCH_HEIGHT = 7.0f;
inline constexpr float BLOCK_NOTCH_OFFSET = 30.0f;
inline constexpr float BLOCK_NOTCH_SLOPE = 6.0f;
inline constexpr float BLOCK_MIN_WIDTH = BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH + 2 * BLOCK_HPAD;
inline constexpr float BLOCK_HEIGHT = 35.0f;
inline constexpr float BLOCK_INPUT_HEIGHT = BLOCK_HEIGHT - 7.0f;
inline constexpr float BLOCK_STR_INPUT_MAXW = 120.0f;
inline constexpr float BLOCK_INT_INPUT_MAXW = 40.0f;
inline constexpr float BLOCK_FLOAT_INPUT_MAXW = 60.0f;

inline constexpr ImU32 CATEGORY_EVENT_COLOR = IM_COL32(49, 196, 49, 255);
inline constexpr ImU32 CATEGORY_CONSOLE_COLOR = IM_COL32(49, 96, 146, 255);
inline constexpr ImU32 CATEGORY_MATH_COLOR = IM_COL32(113, 49, 196, 255);
inline constexpr ImU32 CATEGORY_LOGIC_COLOR = IM_COL32(49, 167, 196, 196);

inline constexpr float OUTPUT_PANEL_HEIGHT = 220.0f;
