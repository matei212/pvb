#include <cassert>
#include <cmath>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <fstream>

#include "ui.hpp"
#include "log.hpp"
#include "build.hpp"

// Block
static void drawBlockShape(float x, float y, float width, float height, BlockType type = BlockType::Instruction, BlockCategory category = BlockCategory::Event);
static void drawBlockTokens(ImVec2 cursorPos, std::vector<BlockToken> &tokens);
static void drawLiteralInput(ImVec2 cursorPos, float width, uint32_t id, uint32_t inputIndex, std::string &literal, ValueType type);
static void drawEmbeddedExpressionBlock(Canvas &canvas, BlockInstance &expr, ImVec2 cursorPos, float slotW, float slotH);
static void drawCanvasTokens(Canvas &canvas, BlockInstance &block, ImVec2 cursorPos);

static ImVec2 blockTextOrigin(ImVec2 pos, const BlockDefinition *def, float blockHeight);
static float calcLiteralWidth(const char *text, ValueType type);

static ImVec2 calcSidebarBlockSize(const BlockData &data);
static float calcTokenWidth(const BlockToken &tok);
static float calcSidebarInputWidth(const char *text, float minW, float maxW);

static float calcCanvasInputWidth(Canvas &canvas, const InputValue &input);
static ImVec2 calcCanvasBlockSize(Canvas &canvas, const BlockInstance &block);

static bool rectsOverlap(ImVec2 aMin, ImVec2 aMax, ImVec2 bMin, ImVec2 bMax);

static ImU32 getCategoryColor(BlockCategory category);

static BlockToken parseBlockInput(const char **ch, const char * end);
static ValueType parseValueType(const std::string &str);

static std::vector<InputValue> buildDefaultInputs(const BlockData &data);

static bool writeTextFile(const std::string& path, const std::string& content);


BlockData::BlockData(const BlockDefinition *def)
    : definition(def)
{
    assert(def != nullptr);
    assert(!def->nameFmt.empty());

    // TODO: Extract this into a separate function
    const std::string &str = def->nameFmt;
    const char *ch = str.data();
    const char *end = ch + str.size();

    while (ch != end) {
        if (*ch == '{') {
            tokens.push_back(parseBlockInput(&ch, end));
        } else {
            if (tokens.empty() || tokens.back().type != BlockTokenType::Text) {
                tokens.emplace_back();
            }

            BlockToken &last = tokens.back();
            last.type = BlockTokenType::Text;
            last.text += *ch;
        }

        ch ++;
    }

#ifndef NDEBUG
    for (auto &tok : tokens) {
        if (tok.type == BlockTokenType::Input) {
            LOG_DEBUG("BlockToken type=input inputName=%s defaultValue=%s", tok.inputName.c_str(), tok.defaultValue.c_str());
        } else if (tok.type == BlockTokenType::Text) {
            LOG_DEBUG("BlockToken type=text text=%s", tok.text.c_str());
        } else {
            LOG_WARN("BlockToken shouldn't have type %d", static_cast<int>(tok.type));
        }
    }
#endif

}

void DrawSidebarBlock(const BlockData &data, UIEventQueue &events)
{
    ImGui::PushID(&data);

    ImVec2 size = calcSidebarBlockSize(data);
    ImGui::Dummy(size);
    ImVec2 pos = ImGui::GetItemRectMin();
    bool hovered = ImGui::IsItemHovered();

    ImVec2 textOffset = blockTextOrigin(pos, data.definition, BLOCK_HEIGHT);
    drawBlockShape(pos.x, pos.y, size.x, size.y, data.definition->type, data.definition->category);
    drawBlockTokens(textOffset, const_cast<std::vector<BlockToken>&>(data.tokens));

    if (hovered) {
        ImGui::SetTooltip("%s", data.definition->description.c_str());

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            events.Push({ UIEventType::BlockInstanciateRequested, 0, &data });
        }
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SIDEBAR_ITEM_VSPACE);

    ImGui::PopID();
}

ImVec2 calcSidebarBlockSize(const BlockData &data)
{
    float width = BLOCK_HPAD * 2.0f;

    for (size_t i = 0; i < data.tokens.size(); i++) {
        width += calcTokenWidth(data.tokens[i]);

        if (i + 1 < data.tokens.size()) {
            width += BLOCK_HSPACE;
        }
    }

    width = std::max(width, BLOCK_MIN_WIDTH);

    return ImVec2(width, BLOCK_HEIGHT);
}

ImVec2 blockTextOrigin(ImVec2 pos, const BlockDefinition *def, float blockHeight)
{
    ImVec2 textSize = ImGui::CalcTextSize(def->nameFmt.c_str());

    return ImVec2(
            pos.x + BLOCK_HPAD,
            pos.y + (blockHeight - textSize.y) * 0.5f);
}

float calcLiteralWidth(const char *text, ValueType type)
{
    if (type & Value_String) {
        return calcSidebarInputWidth(
                text,
                BLOCK_STR_INPUT_MINW,
                BLOCK_STR_INPUT_MAXW);
    } else if (type == Value_Int) {
        return calcSidebarInputWidth(
                text,
                BLOCK_INT_INPUT_MINW,
                BLOCK_INT_INPUT_MAXW);
    } else if (type & Value_Number) {
        return calcSidebarInputWidth(
                text,
                BLOCK_FLOAT_INPUT_MINW,
                BLOCK_FLOAT_INPUT_MAXW);
    } else if (type == Value_Bool) {
        return ImGui::GetFrameHeight(); // checkbox size
    }

    return 0.0f;
}

void drawBlockShape(float x, float y, float width, float height, BlockType type, BlockCategory category)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->PathLineTo(ImVec2(x, y));
    if (type != BlockType::Expression && type != BlockType::Event) {
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET, y));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_SLOPE, y + BLOCK_NOTCH_HEIGHT));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH - BLOCK_NOTCH_SLOPE, y + BLOCK_NOTCH_HEIGHT));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH, y));
    }
    drawList->PathLineTo(ImVec2(x + width, y));
    drawList->PathLineTo(ImVec2(x + width, y + height));
    if (type != BlockType::Expression) {
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH, y + height));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH - BLOCK_NOTCH_SLOPE, y + height + BLOCK_NOTCH_HEIGHT));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_SLOPE, y + height + BLOCK_NOTCH_HEIGHT));
        drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET, y + height));
    }
    drawList->PathLineTo(ImVec2(x, y + height));
    drawList->PathLineTo(ImVec2(x, y));
    drawList->PathFillConcave(getCategoryColor(category));
}

void drawBlockTokens(ImVec2 cursorPos, std::vector<BlockToken> &tokens)
{
    for (size_t i = 0; i < tokens.size(); i ++) {
        BlockToken &tok = tokens[i];
        ImGui::SetCursorScreenPos(cursorPos);

        float width = calcTokenWidth(tok);
        if (tok.type == BlockTokenType::Text) {
            ImGui::TextUnformatted(tok.text.c_str());
        } else if (tok.acceptedTypes & Value_String) {
            ImGui::SetNextItemWidth(width);
            ImGui::PushID(static_cast<int>(i));
            ImGui::InputText("##s", &tok.defaultValue);
            ImGui::PopID();
        } else if (tok.acceptedTypes == Value_Int) {
            ImGui::SetNextItemWidth(width);
            ImGui::PushID(static_cast<int>(i));
            int value = tok.defaultValue.empty() ? 0 : std::stoi(tok.defaultValue);
            if (ImGui::InputScalar("##i", ImGuiDataType_S32, &value, nullptr, nullptr)) {
                tok.defaultValue = std::to_string(value);
            }
            ImGui::PopID();
        } else if (tok.acceptedTypes & Value_Number) {
            ImGui::SetNextItemWidth(width);
            ImGui::PushID(static_cast<int>(i));
            float value = tok.defaultValue.empty() ? 0 : std::stof(tok.defaultValue);
            if (ImGui::InputScalar( "##f", ImGuiDataType_Float, &value, nullptr, nullptr, "%g")) {
                tok.defaultValue = std::to_string(value);
            }
            ImGui::PopID();
        } else if (tok.acceptedTypes == Value_Bool) {
            ImGui::PushID(static_cast<int>(i));

            bool value =
                tok.defaultValue == "true" ||
                tok.defaultValue == "1";

            if (ImGui::Checkbox("##b", &value)) {
                tok.defaultValue = value ? "true" : "false";
            }

            ImGui::PopID();
        }
        cursorPos.x += width + BLOCK_HSPACE;
    }
}

void drawLiteralInput(ImVec2 cursorPos, float width, uint32_t id, uint32_t inputIndex, std::string &literal, ValueType type)
{
    ImGui::SetCursorScreenPos(cursorPos);
    ImGui::SetNextItemWidth(width);
    ImGui::PushID(id);
    ImGui::PushID(inputIndex);

    if (type & Value_String) {
        ImGui::InputText("##s", &literal);
    } else if (type == Value_Int) {
        int value = literal.empty() ? 0 : std::stoi(literal);
        if (ImGui::InputScalar("##i", ImGuiDataType_S32, &value, nullptr, nullptr)) {
            literal = std::to_string(value);
        }
    } else if (type & Value_Number) {
        float value = literal.empty() ? 0 : std::stof(literal);
        if (ImGui::InputScalar( "##f", ImGuiDataType_Float, &value, nullptr, nullptr, "%g")) {
            literal = std::to_string(value);
        }
    } else if (type == Value_Bool) {
        bool value =
            literal == "true" ||
            literal == "1";

        if (ImGui::Checkbox("##b", &value)) {
            literal = value ? "true" : "false";
        }
    }

    ImGui::PopID();
    ImGui::PopID();
}

void drawEmbeddedExpressionBlock(Canvas &canvas, BlockInstance &expr, ImVec2 cursorPos, float, float slotHeight)
{
    expr.size = calcCanvasBlockSize(canvas, expr);
    ImVec2 drawPos = ImVec2(cursorPos.x, cursorPos.y + (slotHeight - expr.size.y) * 0.5f);
    drawBlockShape(drawPos.x, drawPos.y, expr.size.x, expr.size.y, expr.data.definition->type, expr.data.definition->category);
    drawCanvasTokens(canvas, expr, blockTextOrigin(drawPos, expr.data.definition, expr.size.y));
}

static void drawCanvasTokens(Canvas &canvas, BlockInstance &block, ImVec2 cursorPos)
{
    size_t inputIndex = 0;

    for (size_t i = 0; i < block.data.tokens.size(); i++) {
        const BlockToken &tok = block.data.tokens[i];
        ImGui::SetCursorScreenPos(cursorPos);

        if (tok.type == BlockTokenType::Text) {
            ImVec2 size = ImGui::CalcTextSize(tok.text.c_str());
            ImGui::TextUnformatted(tok.text.c_str());
            cursorPos.x += size.x + BLOCK_HSPACE;
            continue;
        }

        InputValue &input = block.inputs[inputIndex];
        float width = calcCanvasInputWidth(canvas, input);

        if (input.connectedBlockId != 0) {
            auto child = canvas.FindBlockById(input.connectedBlockId);
            if (child != canvas.GetBlocks().end()) {
                ImVec2 blockScreenTop = canvas.WorldToScreen(block.pos);
                float blockTop = blockScreenTop.y;
                ImVec2 exprPos = ImVec2(cursorPos.x, blockTop);
                drawEmbeddedExpressionBlock(
                        canvas,
                        *child,
                        exprPos,
                        width,
                        block.size.y);
            }
        } else {
            drawLiteralInput(cursorPos, width, block.id, static_cast<uint32_t>(inputIndex), input.literal, input.type);
        }

        cursorPos.x += width + BLOCK_HSPACE;
        inputIndex++;
    }
}

static ImU32 getCategoryColor(BlockCategory category)
{
    switch (category) {
        case BlockCategory::Event:
            return CATEGORY_EVENT_COLOR;
        case BlockCategory::Console:
            return CATEGORY_CONSOLE_COLOR;
        case BlockCategory::ControlFlow:
            return CATEGORY_CONTROL_COLOR;
        case BlockCategory::Math:
            return CATEGORY_MATH_COLOR;
        case BlockCategory::Logic:
            return CATEGORY_LOGIC_COLOR;
        default:
            LOG_WARN("unknown category %d", static_cast<int>(category));
            return BLOCK_COLOR;
    }
}

static BlockToken parseBlockInput(const char **ch, const char * end)
{
    BlockToken tok;
    tok.type = BlockTokenType::Input;

    std::string typeStr;
    std::string nameStr;
    std::string valStr;

    enum class ParseState
    {
        Type,
        Name,
        Value,
    };

    ParseState state = ParseState::Type;

    (*ch)++;

    while (*ch != end && **ch != '}') {
        char c = **ch;

        if (c == ':') {
            state = ParseState::Name;
            (*ch)++;
            continue;
        }

        if (c == '=') {
            state = ParseState::Value;
            (*ch)++;
            continue;
        }

        switch (state) {
            case ParseState::Type:
                typeStr += c;
                break;

            case ParseState::Name:
                nameStr += c;
                break;

            case ParseState::Value:
                valStr += c;
                break;
        }

        (*ch)++;
    }

    tok.acceptedTypes = parseValueType(typeStr);
    tok.inputName = nameStr;
    tok.defaultValue = valStr;
    return tok;
}

static ValueType parseValueType(const std::string &str)
{
    if (str == "int") return Value_Int;
    if (str == "float") return Value_Float;
    if (str == "number") return Value_Number;
    if (str == "bool") return Value_Bool;
    if (str == "string") return Value_String;
    if (str == "any") return Value_Any;

    return Value_None;
}

static std::vector<InputValue> buildDefaultInputs(const BlockData &data)
{
    std::vector<InputValue> result;

    for (const BlockToken& tok : data.tokens) {
        if (tok.type != BlockTokenType::Input)
            continue;

        InputValue input;
        input.type = tok.acceptedTypes;
        input.literal = tok.defaultValue;

        result.push_back(std::move(input));
    }

    return result;
}

static bool writeTextFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path);
    if (!out.is_open())
        return false;

    out << content;
    return true;
}

static float calcTokenWidth(const BlockToken &tok)
{
    if (tok.type == BlockTokenType::Text) {
        return ImGui::CalcTextSize(tok.text.c_str()).x;
    } else if (tok.type == BlockTokenType::Input) {
        return calcLiteralWidth(tok.defaultValue.c_str(), tok.acceptedTypes);
    }

    return 0.0f;
}

static float calcSidebarInputWidth(const char *text, float minW, float maxW)
{
    ImGuiStyle &style = ImGui::GetStyle();

    float textW = ImGui::CalcTextSize(text).x;

    float width =
        textW +
        style.FramePadding.x * 2.0f +
        8.0f; // breathing room

    return std::clamp(width, minW, maxW);
}

float calcCanvasInputWidth(Canvas &canvas, const InputValue &input)
{
    if (input.connectedBlockId != 0) {
        auto it = canvas.FindBlockById(input.connectedBlockId);

        if (it != canvas.GetBlocks().end()) {
            return it->size.x;
        }
    }

    return calcLiteralWidth(input.literal.c_str(), input.type);
}

static ImVec2 calcCanvasBlockSize(Canvas &canvas, const BlockInstance &block)
{
    float width = BLOCK_HPAD * 2.0f;
    float height = BLOCK_HEIGHT;

    size_t inputIndex = 0;

    for (const BlockToken& tok : block.data.tokens) {
        if (tok.type == BlockTokenType::Text) {
            width += ImGui::CalcTextSize(tok.text.c_str()).x;
        } else {
            const InputValue &input = block.inputs[inputIndex];
            width += calcCanvasInputWidth(canvas, input);

             if (input.connectedBlockId != 0) {
                auto child = canvas.FindBlockById(input.connectedBlockId);

                if (child != canvas.GetBlocks().end()) {
                    child->size = calcCanvasBlockSize(canvas, *child);

                    float needed =
                        child->size.y +
                        BLOCK_VPAD * 2.0f +
                        BLOCK_NOTCH_HEIGHT;

                    height = std::max(height, needed);
                }
            }

            inputIndex++;
        }

        width += BLOCK_HSPACE;
    }

    width = std::max(width, BLOCK_MIN_WIDTH);
    return ImVec2(width, height);
}

static bool rectsOverlap(ImVec2 aMin, ImVec2 aMax, ImVec2 bMin, ImVec2 bMax)
{
    return aMin.x <= bMax.x
        && aMax.x >= bMin.x
        && aMin.y <= bMax.y
        && aMax.y >= bMin.y;
}

// Block Instance
void DrawCanvasBlock(Canvas &canvas, BlockInstance &block, UIEventQueue &events)
{
    assert(block.data.definition != nullptr);

    if (block.isDragging) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::SetNextWindowPos(canvas.WorldToScreen(block.pos), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(block.size.x, block.size.y + BLOCK_NOTCH_HEIGHT));

        std::string name = "DraggingBlock" + std::to_string(block.id);
        ImGui::Begin(name.c_str(), nullptr,
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_NoScrollWithMouse
                | ImGuiWindowFlags_NoInputs
                | ImGuiWindowFlags_NoDecoration);
    }

    ImGui::PushID(static_cast<int>(block.id));

    ImVec2 screenPos = canvas.WorldToScreen(block.pos);
    block.size = calcCanvasBlockSize(canvas, block);
    drawBlockShape(
            screenPos.x,
            screenPos.y,
            block.size.x,
            block.size.y,
            block.data.definition->type,
            block.data.definition->category);

    drawCanvasTokens(
            canvas,
            block,
            blockTextOrigin(screenPos, block.data.definition, block.size.y));

    ImGui::SetCursorScreenPos(screenPos);
    ImGui::InvisibleButton("##block", block.size);
    block.isActive = ImGui::IsItemActive();
    block.isHovered = ImGui::IsItemHovered();

    block.isMenuOpen = ImGui::BeginPopupContextItem("##ctx", ImGuiMouseButton_Right);
    if (block.isMenuOpen) {
        if (ImGui::MenuItem("Duplicate")) events.Push({ UIEventType::BlockDuplicateReqested, block.id, nullptr });
        if (ImGui::MenuItem("Delete")) events.Push({ UIEventType::BlockDeleteReqested, block.id, nullptr });

        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (block.isDragging) {
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void UpdateCanvasBlock(Canvas &canvas, BlockInstance &block, UIEventQueue &events)
{
    if (block.isMenuOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = canvas.ScreenToWorld(io.MousePos);

    if (!block.isDragging && block.isActive) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            block.mouseDownPos = mousePos;
        }

        if (block.isMouseDown && io.MouseDown[ImGuiMouseButton_Left]) {
            float dx = mousePos.x - block.mouseDownPos.x;
            float dy = mousePos.y - block.mouseDownPos.y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist >= BLOCK_DRAG_THRESH) {
                block.isDragging = true;
                block.dragOffset = ImVec2(mousePos.x - block.pos.x, mousePos.y - block.pos.y);
                block.isMouseDown = false;
                events.Push({ UIEventType::BlockDragStarted, block.id, nullptr });
            }
        }

        block.isMouseDown = io.MouseDown[ImGuiMouseButton_Left];
    }

    if (block.isDragging) {
        if (io.MouseDown[ImGuiMouseButton_Left]) {
            ImVec2 oldPos = block.pos;
            ImVec2 newPos = ImVec2(mousePos.x - block.dragOffset.x, mousePos.y - block.dragOffset.y);
            ImVec2 delta = ImVec2(newPos.x - oldPos.x, newPos.y - oldPos.y);
            events.Push({ .type = UIEventType::BlockDragged, .id = block.id, .delta = delta });
        } else {
            block.isDragging = false;
            events.Push({ UIEventType::BlockDragEnded, block.id, nullptr });
        }
    }
}


// Sidebar
// TODO: THIS IS SUPER UNSAFE
void Sidebar::Init()
{
    m_Blocks.reserve(g_BlockDefinitions.size());
    for (auto &definition : g_BlockDefinitions) {
        m_Blocks.emplace_back(&definition);
    }
    LOG_DEBUG("initialized sidebar");
}

void Sidebar::Update()
{
    // for (auto &block : m_Blocks) {
    // block.Update();
    // }
}

void Sidebar::Draw(UIEventQueue &events)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SIDEBAR_PAD, SIDEBAR_PAD));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, SIDEBAR_ITEM_VSPACE));
    ImGui::BeginChild(
            "Sidebar",
            ImVec2(SIDEBAR_WIDTH, 0.0f),
            ImGuiChildFlags_AlwaysUseWindowPadding);

    for (auto &block : m_Blocks) {
        DrawSidebarBlock(block, events);
    }

    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}


// Canvas
void Canvas::Init()
{
    LOG_DEBUG("Canvas Initialized");
}

void Canvas::Update(UIEventQueue &events)
{
    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        m_IsPanning = true;
        m_LastMousePos = io.MousePos;
    }

    if (m_IsPanning) {
        ImVec2 delta(io.MousePos.x - m_LastMousePos.x, io.MousePos.y - m_LastMousePos.y);
        m_PanOffset.x += delta.x;
        m_PanOffset.y += delta.y;
        m_LastMousePos = io.MousePos;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        m_IsPanning = false;
    }

    for (auto &block : m_Blocks) {
        UpdateCanvasBlock(*this, block, events);
    }
}

void Canvas::Draw(UIEventQueue &events)
{
    ImVec2 canvasSize(
            std::max(380.0f, ImGui::GetContentRegionAvail().x),
            std::max(260.0f, ImGui::GetContentRegionAvail().y));


    ImGui::BeginChild("CanvasFrame", canvasSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Grid
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    for (float x = fmodf(m_PanOffset.x, GRID_STEP); x < winSize.x; x += GRID_STEP) {
        drawList->AddLine(
                ImVec2(winPos.x + x, winPos.y),
                ImVec2(winPos.x + x, winPos.y + winSize.y),
                GRID_COLOR);
    }

    for (float y = fmodf(m_PanOffset.y, GRID_STEP); y < winSize.y; y += GRID_STEP) {
        drawList->AddLine(
                ImVec2(winPos.x, winPos.y + y),
                ImVec2(winPos.x + winSize.x, winPos.y + y),
                GRID_COLOR);
    }

    // Blocks
    for (auto &block : m_Blocks) {
        if (block.parentBlockId != 0) continue;
        DrawCanvasBlock(*this, block, events);

#ifndef NDEBUG
        if (block.isHovered) {
            ImGui::SetTooltip("Debug: id=%u, prevId=%u, nextId=%u, parentId=%u", block.id, block.prevId, block.nextId, block.parentBlockId);
        }
#endif

    }
    ImGui::EndChild();

}

void Canvas::InstanceBlock(const BlockData &data)
{
    ImGuiIO &io = ImGui::GetIO();

    ImVec2 spawnPos = ScreenToWorld(io.MousePos);

    BlockInstance instance {
        .id         = m_NextId++,
            .data       = data,
            .pos        = spawnPos,
            .size       = calcSidebarBlockSize(data),
            .inputs     = buildDefaultInputs(data),
            .isDragging = true,
    };
    m_Blocks.push_back(std::move(instance));
    BlockInstance &inst = m_Blocks.back();
    LOG_DEBUG("instanced block id=%u at (%.0f, %.0f)", inst.id, inst.pos.x, inst.pos.y);
}

void Canvas::DuplicateInstance(uint32_t id)
{
    auto original = FindBlockById(id);
    BlockInstance inst {
        .id         = m_NextId++,
    .data       = original->data,
    .pos        = ImVec2(original->pos.x + 40.0f, original->pos.y + 40.0f),
    .size       = calcSidebarBlockSize(original->data),
    .inputs     = buildDefaultInputs(original->data),
};
    m_Blocks.push_back(std::move(inst));
    LOG_DEBUG("duplicated instance id=%u at (%.0f, %.0f) with id=%u", id, inst.pos.x, inst.pos.y, inst.id);
}

void Canvas::DeleteInstance(uint32_t id)
{
    int32_t idx = FindIdxById(id);
    if (idx == -1) {
        LOG_ERROR("failed to find block with id %u", id);
        return;
    }

    BlockInstance &inst = m_Blocks[idx];
    if (inst.prevId != 0) {
        auto prev = FindBlockById(inst.prevId);
        prev->nextId = 0;
    }
    if (inst.parentBlockId != 0) {
        auto parent = FindBlockById(inst.parentBlockId);
        if (parent != m_Blocks.end()
            && inst.parentInputIndex >= 0
            && static_cast<size_t>(inst.parentInputIndex) < parent->inputs.size()) { 
            parent->inputs[inst.parentInputIndex].connectedBlockId = 0;
        }
    }

    m_Blocks.erase(m_Blocks.begin() + idx);
    LOG_DEBUG("deleted block with id %u", id);
}

void Canvas::AttachInstance(uint32_t id, AttachTarget target)
{
    auto instance = FindBlockById(id);
    auto other = FindBlockById(target.id);

    ImVec2 newPos = ImVec2(target.pos.x - BLOCK_NOTCH_OFFSET, target.pos.y);
    if (target.type == AttachType::TopNotch)  {
        newPos.y -= target.size.y;
    }
    ImVec2 delta = ImVec2(newPos.x - instance->pos.x, newPos.y - instance->pos.y);
    WalkBlockSequence(id, [&delta](BlockInstance &inst) {
            inst.pos.x += delta.x;
            inst.pos.y += delta.y;
            });

    if (target.type == AttachType::BottomNotch) {
        other->nextId = id;
        instance->prevId = other->id;
    } else if (target.type == AttachType::TopNotch) {
        other->prevId = id;
        instance->nextId = other->id;
    }

    LOG_DEBUG("attached block id=%u to block id=%u", id, other->id);
}

void Canvas::DetachInstane(uint32_t id)
{
    auto inst = FindBlockById(id);
    if (inst->prevId == 0) {
        LOG_ERROR("tried to detach block id=%u with no parent", id);
    }

    auto prev = FindBlockById(inst->prevId);
    prev->nextId = 0;
    inst->prevId = 0;
    LOG_DEBUG("detached block id=%u from block id=%u", id, prev->id);
}

void Canvas::AttachExpressionBlock(uint32_t exprId, uint32_t parentId, uint32_t inputIndex)
{
    auto expr = FindBlockById(exprId);
    auto parent = FindBlockById(parentId);
    if (expr == m_Blocks.end() || parent == m_Blocks.end()) {
        LOG_ERROR("failed to attach expression block id=%u to parent id=%u", exprId, parentId);
        return;
    }

    if (inputIndex >= parent->inputs.size()) {
        LOG_ERROR("input index %u out of range for block id=%u", inputIndex, parentId);
        return;
    }

    expr->parentBlockId = parentId;
    expr->parentInputIndex = inputIndex;
    parent->inputs[inputIndex].connectedBlockId = exprId;

    LOG_DEBUG("attached expression block id=%u to block id=%u input %u", exprId, parentId, inputIndex);
}

void Canvas::DetachExpressionBlock(uint32_t exprId)
{
    auto expr = FindBlockById(exprId);
    if (expr == m_Blocks.end()) return;

    if (expr->parentBlockId == 0) return;

    auto parent = FindBlockById(expr->parentBlockId);
    if (parent != m_Blocks.end()
            && expr->parentInputIndex >= 0
            && static_cast<size_t>(expr->parentInputIndex) < parent->inputs.size()) {
        parent->inputs[expr->parentInputIndex].connectedBlockId = 0;
    }

    expr->parentBlockId = 0;
    expr->parentInputIndex = -1;

    LOG_DEBUG("detached expression block id=%u", exprId);
}

void Canvas::WalkBlockSequence(uint32_t id, std::function<void (BlockInstance &inst)> callback)
{
    std::unordered_set<uint32_t> visited;

    uint32_t current = id;
    while (current != 0) {
        if (visited.find(current) != visited.end()) {
            LOG_ERROR("cycle detected at %u", current);
            break;
        }
        visited.insert(current);

        auto inst = FindBlockById(current);
        if (inst == m_Blocks.end()) {
            LOG_ERROR("block %u not found", id);
            return;
        }

        current = inst->nextId;
        callback(*inst);
    }
}

void Canvas::MoveAttachedExpression(BlockInstance &parent)
{
    ImVec2 cursor = blockTextOrigin(parent.pos, parent.data.definition, parent.size.y);

    size_t inputIndex = 0;

    for (const BlockToken &tok : parent.data.tokens) {
        if (tok.type == BlockTokenType::Text) {
            cursor.x += ImGui::CalcTextSize(tok.text.c_str()).x + BLOCK_HSPACE;
            continue;
        }

        InputValue &input = parent.inputs[inputIndex];
        float width = calcCanvasInputWidth(*this, input);

        if (input.connectedBlockId != 0) {
            auto child = FindBlockById(input.connectedBlockId);

            if (child != m_Blocks.end()) {
                child->size = calcCanvasBlockSize(*this, *child);

                child->pos = ImVec2(
                        cursor.x,
                        cursor.y + (parent.size.y - child->size.y) * 0.5f
                        );

                MoveAttachedExpression(*child);
            }
        }

        cursor.x += width + BLOCK_HSPACE;
        inputIndex++;
    }
}

void Canvas::CollectBlockTree(uint32_t id, std::unordered_set<uint32_t>& visited)
{
    if (id == 0) return;

    if (visited.find(id) != visited.end())
        return;

    auto inst = FindBlockById(id);
    if (inst == m_Blocks.end())
        return;

    visited.insert(id);

    // Follow statement chain
    if (inst->nextId != 0) {
        CollectBlockTree(inst->nextId, visited);
    }

    // Follow expression children
    for (const InputValue& input : inst->inputs) {
        if (input.connectedBlockId != 0) {
            CollectBlockTree(input.connectedBlockId, visited);
        }
    }
}

std::optional<BlockInstance> Canvas::GetMainBlock()
{
    for (auto &block : m_Blocks) {
        if (block.data.definition->type == BlockType::Event
            && block.data.definition->nameFmt == "Main") {
            return block;
        }
    }

    return std::nullopt;
}

void Canvas::AppendBlockInputSockets(BlockInstance &block)
{
    block.size = calcCanvasBlockSize(*this, block);

    ImVec2 cursor = blockTextOrigin(block.pos, block.data.definition, block.size.y);
    size_t inputIndex = 0;

    for (const BlockToken &tok : block.data.tokens) {
        if (tok.type == BlockTokenType::Text) {
            cursor.x += ImGui::CalcTextSize(tok.text.c_str()).x + BLOCK_HSPACE;
            continue;
        }

        float width = calcCanvasInputWidth(*this, block.inputs[inputIndex]);

        InputSocket socket;
        socket.pos = cursor;
        socket.size = ImVec2(width, block.size.y);
        socket.ownerBlockId = block.id;
        socket.inputIndex = static_cast<uint32_t>(inputIndex);
        socket.acceptedTypes = tok.acceptedTypes;
        m_InputSockets.push_back(socket);

        cursor.x += width + BLOCK_HSPACE;
        inputIndex++;
    }
}

void Canvas::RebuildInputSockets()
{
    m_InputSockets.clear();

    for (auto &block : m_Blocks) {
        if (block.parentBlockId != 0) continue;
        AppendBlockInputSockets(block);
    }
}

ImVec2 Canvas::WorldToScreen(ImVec2 pos)
{
    return ImVec2(pos.x + m_PanOffset.x, pos.y + m_PanOffset.y);
}

ImVec2 Canvas::ScreenToWorld(ImVec2 pos)
{
    return ImVec2(pos.x - m_PanOffset.x, pos.y - m_PanOffset.y);
}

void Canvas::BringToFront(uint32_t id)
{
    int32_t idx = FindIdxById(id);
    if (idx == -1) {
        LOG_ERROR("failed to find block with id %u", id);
        return;
    }

    std::swap(m_Blocks[idx], m_Blocks.back());
}

std::optional<AttachTarget> Canvas::FindAttachTarget(const BlockInstance &instance)
{
    if (instance.data.definition->type == BlockType::Expression) return std::nullopt;

    ImVec2 topNotch = ImVec2(instance.pos.x + BLOCK_NOTCH_OFFSET, instance.pos.y);
    ImVec2 bottomNotch = ImVec2(instance.pos.x + BLOCK_NOTCH_OFFSET, instance.pos.y + instance.size.y);

    for (auto &target : m_Blocks) {
        if (target.id == instance.id) continue;
        if (target.data.definition->type == BlockType::Expression) continue;

        float dx, dy, dist;

        // top notch of instance and bottom notch of target
        if (target.nextId == 0) {
            ImVec2 targetBottomNotch = ImVec2(target.pos.x + BLOCK_NOTCH_OFFSET, target.pos.y + target.size.y);
            dx = topNotch.x - targetBottomNotch.x;
            dy = topNotch.y - targetBottomNotch.y;
            dist = sqrt(dx * dx + dy * dy);
            if (dist <= BLOCK_SNAP_RADIUS) {
                return AttachTarget { target.id, targetBottomNotch, target.size, AttachType::BottomNotch };
            }
        }

        // bottom notch of instance and top notch of target
        if (target.prevId == 0) {
            ImVec2 targetTopNotch = ImVec2(target.pos.x + BLOCK_NOTCH_OFFSET, target.pos.y);
            dx = bottomNotch.x - targetTopNotch.x;
            dy = bottomNotch.y - targetTopNotch.y;
            dist = sqrt(dx * dx + dy * dy);
            if (dist <= BLOCK_SNAP_RADIUS) {
                return AttachTarget { target.id, targetTopNotch, target.size, AttachType::TopNotch };
            }
        }
    }

    return std::nullopt;
}

std::optional<InputSocket> Canvas::FindInputSocketTarget(const BlockInstance &instance)
{
    if (instance.data.definition->type != BlockType::Expression) return std::nullopt;
    if (instance.data.definition->outputType == Value_None) return std::nullopt;

    std::optional<InputSocket> best;
    for (const InputSocket &socket : m_InputSockets) {
        if (socket.ownerBlockId == instance.id) continue;
        if (FindIdxById(socket.ownerBlockId) == -1) continue;

        auto owner = FindBlockById(socket.ownerBlockId);

        if ((socket.acceptedTypes & instance.data.definition->outputType) == 0) continue;
        if (owner->inputs[socket.inputIndex].connectedBlockId != 0
            && owner->inputs[socket.inputIndex].connectedBlockId != instance.id) {
            continue;
        }

        bool overlaps = rectsOverlap(
                instance.pos,
                ImVec2(instance.pos.x + instance.size.x, instance.pos.y + instance.size.y), 
                socket.pos,
                ImVec2(socket.pos.x + socket.size.x, socket.pos.y + socket.size.y));
        if (!overlaps) continue;

        return socket;
    }

    return std::nullopt;
}

std::vector<BlockInstance>::iterator Canvas::FindBlockById(uint32_t id)
{
    return std::find_if(
            m_Blocks.begin(),
            m_Blocks.end(),
            [id](const BlockInstance& b) {
            return b.id == id;
            }
            );
}

int32_t Canvas::FindIdxById(uint32_t id)
{
    int32_t idx = -1;
    for (size_t i = 0; i < m_Blocks.size(); i ++) {
        if (id == m_Blocks[i].id) {
            idx = static_cast<int32_t>(i);
            break;
        }
    }

    return idx;
}


// CodeView
void CodeView::Generate(Canvas &canvas)
{
    auto main = canvas.GetMainBlock();
    if (!main.has_value()) {
        LOG_ERROR("no main block in program");
        return;
    }

    ASTBuilder builder;
    auto ast = builder.build(main.value(), canvas);
    CodeGen generator;
    m_Code = generator.emit(*ast);
}

void CodeView::Draw()
{
    ImGui::BeginChild("CodeView", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::SetWindowFontScale(1.3f);
    ImGui::InputTextMultiline(
            "##code",
            &m_Code,
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_ReadOnly);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::PopStyleVar();
    ImGui::EndChild();
}


// UI
void UI::Init()
{
    if (!glfwInit())
        die("failed to init GLFW");
    LOG_DEBUG("initialized GLFW");

    if (!(m_Window = glfwCreateWindow(1280, 820, "PVB", nullptr, nullptr))) {
        glfwTerminate();
        die("failed to create window");
    }
    LOG_DEBUG("created window");

    glfwMakeContextCurrent(m_Window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Setup renderer backend
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init();
    LOG_DEBUG("initialized ImGui");

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderHoverPadding = 12.0f;

    m_Sidebar.Init();
    m_Canvas.Init();
}

void UI::Update()
{
    if (m_ShowSidebar) m_Sidebar.Update();
    m_Canvas.Update(m_EventQueue);

    for (const UIEvent &e : m_EventQueue.Drain()) {
        switch (e.type) {
            case UIEventType::BlockDragStarted:
                {
                    LOG_DEBUG("drag started for block %u", e.id);
                    m_Canvas.IsDraggingBlock = true;

                    m_Canvas.WalkBlockSequence(
                            e.id,
                            [this](BlockInstance &inst) {
                                m_Canvas.BringToFront(inst.id);
                                m_Canvas.MoveAttachedExpression(inst);
                            });

                    auto inst = m_Canvas.FindBlockById(e.id);
                    if (inst->data.definition->type == BlockType::Expression) {
                        if (inst->parentBlockId != 0) {
                            m_Canvas.DetachExpressionBlock(e.id);
                        }
                    } else if (inst->prevId != 0) {
                        m_Canvas.DetachInstane(e.id);
                    }

                    break;
                }
            case UIEventType::BlockDragged:
                {
                    m_Canvas.WalkBlockSequence(
                            e.id,
                            [this, &e](BlockInstance &inst) { inst.pos.x += e.delta.x; inst.pos.y += e.delta.y; }
                            );
                    break;
                }
            case UIEventType::BlockDragEnded:
                {
                    LOG_DEBUG("drag ended for block %u", e.id);
                    m_Canvas.IsDraggingBlock = false;

                    auto instance = m_Canvas.FindBlockById(e.id);

                    if (instance->data.definition->type != BlockType::Expression) {
                        auto target = m_Canvas.FindAttachTarget(*instance);
                        if (target) {
                            m_Canvas.AttachInstance(e.id, target.value());
                        }
                    } else {
                        m_Canvas.RebuildInputSockets();
                        auto socket = m_Canvas.FindInputSocketTarget(*instance);
                        if (socket) {
                            instance->pos = socket->pos;
                            m_Canvas.AttachExpressionBlock(e.id, socket->ownerBlockId, socket->inputIndex);

                            auto parent = m_Canvas.FindBlockById(socket->ownerBlockId);
                            m_Canvas.MoveAttachedExpression(*parent);
                        }
                        LOG_DEBUG("attached block %u to socket", instance->id);
                    }

                    break;
                }
            case UIEventType::BlockInstanciateRequested:
                {
                    LOG_DEBUG("block instanciation requested");
                    if (!m_Canvas.IsDraggingBlock) {
                        m_Canvas.IsDraggingBlock = true;
                        m_Canvas.InstanceBlock(*e.data);
                    }
                    break;
                }
            case UIEventType::BlockDeleteReqested:
                {
                    LOG_DEBUG("block deletion requested");

                    std::unordered_set<uint32_t> ids;
                    m_Canvas.CollectBlockTree(e.id, ids);
                    for (uint32_t id : ids) {
                        m_Canvas.DeleteInstance(id);
                    }

                    break;
                }
            case UIEventType::BlockDuplicateReqested:
                LOG_DEBUG("block duplication requested");
                m_Canvas.DuplicateInstance(e.id);
                break;
            default:
                LOG_WARN("unknown event type %d", static_cast<int>(e.type));
                break;
        }
    }
    m_EventQueue.Clear();
}

void UI::Draw()
{
    StartFrame();

    DrawMainMenuBar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 pos = viewport->Pos;
    pos.y += ImGui::GetFrameHeight();
    ImVec2 size = viewport->Size;
    size.y -= ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("Root", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

    DrawWorkspace();
    DrawOutputPanel();

    ImGui::PopStyleVar(2);
    ImGui::End();

    Render();
}

void UI::Destroy()
{
    assert(m_Window != nullptr);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_Window);
    glfwTerminate();

    m_Window = nullptr;
}

void UI::StartFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::Render()
{
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::DrawMainMenuBar()
{
    bool showPopup = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Code View", nullptr, &m_ShowCodeView);
            ImGui::MenuItem("Output Panel", nullptr, &m_ShowOutputPanel);
            ImGui::MenuItem("Sidebar", nullptr, &m_ShowSidebar);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Regenerate Code")) {
                m_CodeView.Generate(m_Canvas);
                m_ShowCodeView = true;
            }
            if (ImGui::MenuItem("Build")) {
                m_ShowOutputPanel = true;

        m_CodeView.Generate(m_Canvas);
        if (!writeTextFile(m_BuildSettings.sourceFile, m_CodeView.GetCode())) {
            LOG_ERROR("failed to write file %s", m_BuildSettings.sourceFile.c_str());
        }

        BuildResult result = BuildProgram(m_BuildSettings);
        m_LastBuild = result;
        m_HasBuilt = true;
            }
            if (ImGui::MenuItem("Run")) {
                m_ShowOutputPanel = true;
        if (m_HasBuilt && m_LastBuild.success) {
            m_LastRun = RunProgram(m_BuildSettings.outputBinary);
            m_HasRan = true;
        }
            }

        if (ImGui::MenuItem("Build && Run")) {
                m_ShowOutputPanel = true;
        m_LastBuild = BuildProgram(m_BuildSettings);

        if (m_LastBuild.success)
            m_LastRun = RunProgram(m_BuildSettings.outputBinary);

        m_HasBuilt = true;
        m_HasRan = true;
        }

        if (ImGui::MenuItem("Build Settings")) {
        showPopup = true;
        }

        ImGui::EndMenu();
    }

        ImGui::EndMainMenuBar();
    }

    if (showPopup) {
    ImGui::OpenPopup("Build Settings");
    }
    if (ImGui::BeginPopupModal("Build Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    static BuildSettings tempSettings;

    ImGui::InputText("Compiler Path", &tempSettings.compilerPath);
    ImGui::InputText("Compiler Flags", &tempSettings.compilerFlags);
    ImGui::InputText("Output Binary", &tempSettings.outputBinary);
    ImGui::InputText("Source File", &tempSettings.sourceFile);

    ImGui::Separator();

    if (ImGui::Button("Save")) {
        m_BuildSettings = tempSettings;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    }
}

void UI::DrawWorkspace()
{
    float spacingY = ImGui::GetStyle().ItemSpacing.y;
    float reservedBottomHeight = m_ShowOutputPanel ? OUTPUT_PANEL_HEIGHT + spacingY : 0.0f;
    float workspaceHeight = std::max(260.0f, ImGui::GetContentRegionAvail().y - reservedBottomHeight);

    ImGui::BeginChild("Workspace", ImVec2(0.0f, workspaceHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    if (m_ShowSidebar) {
        m_Sidebar.Draw(m_EventQueue);
        ImGui::SameLine();
    }

    if (m_ShowCodeView) {
        if (ImGui::BeginTable(
                    "WorkspaceSplit",
                    2,
                    ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_NoSavedSettings |
                    ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Canvas", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthStretch, 0.3f);

            ImGui::TableNextColumn();
            m_Canvas.Draw(m_EventQueue);

            ImGui::TableNextColumn();
            m_CodeView.Draw();

            ImGui::EndTable();
        }
    } else {
        m_Canvas.Draw(m_EventQueue);
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void UI::DrawOutputPanel()
{
    if (!m_ShowOutputPanel) return;

    ImGui::BeginChild("OutputPanel", ImVec2(0.0f, OUTPUT_PANEL_HEIGHT), true);
    if (ImGui::BeginTabBar("Output")) {

    if (ImGui::BeginTabItem("Build Log")) {
        if (m_HasBuilt) {
        ImGui::BeginChild("BuildLog", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(m_LastBuild.output.c_str());
        ImGui::EndChild();
        }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Run Log")) {
        if (m_HasRan) {
        ImGui::BeginChild("Ran", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(m_LastRun.output.c_str());
        ImGui::EndChild();
        }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();
}
