#include <cassert>
#include <cmath>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "ui.hpp"
#include "log.hpp"

// Block
static ImVec2 calcBlockSize(const BlockDefinition *def);
static ImVec2 blockTextOrigin(ImVec2 pos, const BlockDefinition *def);
static void drawBlockShape(float x, float y, float width, float height, BlockType type = BlockType::Instruction, BlockCategory category = BlockCategory::Event);
static void drawBlockTokens(ImVec2 cursorPos, std::vector<BlockToken> &tokens);
static ImU32 getCategoryColor(BlockCategory category);
static BlockToken parseBlockInput(const char **ch, const char * end);


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

}

void DrawSidebarBlock(const BlockData &data, UIEventQueue &events)
{
    ImGui::PushID(&data);

    ImVec2 size = calcBlockSize(data.definition);
    ImGui::Dummy(size);
    ImVec2 pos = ImGui::GetItemRectMin();
    bool hovered = ImGui::IsItemHovered();

    ImVec2 textOffset = blockTextOrigin(pos, data.definition);
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

ImVec2 calcBlockSize(const BlockDefinition *def)
{
    ImVec2 textSize = ImGui::CalcTextSize(def->nameFmt.c_str());
    return ImVec2(std::max(textSize.x + BLOCK_HPAD * 2.0f, BLOCK_MIN_WIDTH),
                  BLOCK_HEIGHT);
}

ImVec2 blockTextOrigin(ImVec2 pos, const BlockDefinition *def)
{
    ImVec2 textSize = ImGui::CalcTextSize(def->nameFmt.c_str());
    return ImVec2(pos.x + BLOCK_HPAD, pos.y + (BLOCK_HEIGHT - textSize.y) * 0.5f);
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

        switch (tok.type) {
            case BlockTokenType::Text:
                {
                    ImVec2 size = ImGui::CalcTextSize(tok.text.c_str());
                    ImGui::TextUnformatted(tok.text.c_str());
                    cursorPos.x += size.x + BLOCK_HSPACE;
                    break;
                }

            case BlockTokenType::StringInput:
                {
                    ImGui::SetNextItemWidth(BLOCK_STR_INPUT_MAXW);
                    ImGui::PushID(i);
                    ImGui::InputText("##s", &tok.strValue);
                    ImGui::PopID();

                    cursorPos.x += BLOCK_STR_INPUT_MAXW + BLOCK_HSPACE;
                    break;
                }

            case BlockTokenType::IntInput:
                {
                    ImGui::SetNextItemWidth(BLOCK_INT_INPUT_MAXW);
                    ImGui::PushID(i);
                    ImGui::InputInt(("##i" + std::to_string(i)).c_str(), &tok.intValue, 0);
                    ImGui::PopID();

                    cursorPos.x += BLOCK_INT_INPUT_MAXW + BLOCK_HSPACE;
                    break;
                }

            case BlockTokenType::FloatInput:
                {
                    ImGui::SetNextItemWidth(BLOCK_FLOAT_INPUT_MAXW);
                    ImGui::PushID(i);
                    ImGui::InputFloat(("##f" + std::to_string(i)).c_str(), &tok.floatValue);
                    ImGui::PopID();

                    cursorPos.x += BLOCK_FLOAT_INPUT_MAXW + BLOCK_HSPACE;
                    break;
                }
        }
    }
}

static ImU32 getCategoryColor(BlockCategory category)
{
    switch (category) {
        case BlockCategory::Event:
            return CATEGORY_EVENT_COLOR;
        case BlockCategory::Console:
            return CATEGORY_CONSOLE_COLOR;
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
    BlockToken token;

    std::string typeStr;
    std::string valStr;
    std::string *str = &typeStr;

    (*ch)++;
    while (*ch != end && **ch != '}') {
        if (str == &typeStr && std::isspace(**ch)) {
            ch ++;
            continue;
        }

        if (**ch == '=') {
            str = &valStr;
            (*ch)++;
            continue;
        }

        *str += **ch;
        (*ch)++;
    }

    if (typeStr == "str") {
        token.type = BlockTokenType::StringInput;
        token.strValue = valStr;
    } else if (typeStr == "int") {
        token.type = BlockTokenType::IntInput;
        token.intValue = std::stoi(valStr);
    } else if (typeStr == "float") {
        token.type = BlockTokenType::FloatInput;
        token.floatValue = std::stof(valStr);
    }else {
        LOG_WARN("unknown block input type %s", typeStr.c_str());
    }

    return token;
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
    block.size = calcBlockSize(block.data.definition);
    drawBlockShape(
            screenPos.x,
            screenPos.y,
            block.size.x,
            block.size.y,
            block.data.definition->type,
            block.data.definition->category);

    drawBlockTokens(
            blockTextOrigin(screenPos, block.data.definition),
            block.data.tokens
    );

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
        DrawCanvasBlock(*this, block, events);

#ifndef NDEBUG
        if (block.isHovered) {
            ImGui::SetTooltip("Debug: id=%u, prevId=%u, nextId=%u", block.id, block.prevId, block.nextId);
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
        .size       = calcBlockSize(data.definition),
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
        .pos        = ImVec2(original->pos.x + 40.0, original->pos.y + 40.0),
        .size       = calcBlockSize(original->data.definition),
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
            idx = i;
            break;
        }
    }

    return idx;
}


// CodeView
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
                            }
                            );

                    auto inst = m_Canvas.FindBlockById(e.id);
                    if (inst->prevId != 0) {
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
                    auto target = m_Canvas.FindAttachTarget(*instance);
                    if (target) {
                        m_Canvas.AttachInstance(e.id, target.value());
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

                    std::vector<uint32_t> ids;
                    auto inst = m_Canvas.FindBlockById(e.id);
                    m_Canvas.WalkBlockSequence(
                            inst->id,
                            [this, &ids](BlockInstance &inst) {
                                ids.push_back(inst.id);
                            });

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
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Code View", nullptr, &m_ShowCodeView);
            ImGui::MenuItem("Output Panel", nullptr, &m_ShowOutputPanel);
            ImGui::MenuItem("Sidebar", nullptr, &m_ShowSidebar);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Regenerate Code")) { }
            if (ImGui::MenuItem("Build")) {
                m_ShowOutputPanel = true;
            }
            if (ImGui::MenuItem("Run")) {
                m_ShowOutputPanel = true;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
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
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Run Log")) {
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();
}
