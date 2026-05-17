#include <cassert>
#include <cmath>
#include <cctype>
#include <iostream>

#include "ui.hpp"
#include "log.hpp"


// Block
static void drawBlockShape(float x, float y, float width, float height);
static void drawBlockTokens(ImVec2 cursorPos, std::vector<BlockToken> &tokens);
static BlockToken parseBlockInput(const char **ch, const char * end);

Block::Block(const BlockDefinition* definition)
    : m_Definition(definition)
{
    assert(m_Definition != nullptr);

    // Parse name
    const std::string &str = definition->nameFmt;
    const char *ch = str.data();
    const char *end = ch + str.size();

    while (ch != end) {
        if (*ch == '{') {
            m_Tokens.push_back(parseBlockInput(&ch, end));
        } else {
            if (m_Tokens.empty() || m_Tokens.back().type != BlockTokenType::Text) {
                m_Tokens.emplace_back();
            }

            BlockToken &last = m_Tokens.back();
            last.type = BlockTokenType::Text;
            last.text += *ch;
        }

        ch ++;
    }
}

void Block::Update()
{
    assert(m_Definition != nullptr);
    assert(OnStartDrag != nullptr);

    if (IsHovered()) {
        bool mouseDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);

        if (mouseDragging && !m_IsDragging) {
            m_IsDragging = true;
            OnStartDrag();
        }

        if (!mouseDragging && m_IsDragging) {
            m_IsDragging = false;
        }
    }
}

void Block::Draw()
{
    assert(m_Definition != nullptr);
    ImGui::PushID(static_cast<int>(m_Definition->type));

    // Reserve space in layout
    ImGui::Dummy(m_Size);
    m_Pos = ImGui::GetItemRectMin();
    UpdateSize();

    drawBlockShape(m_Pos.x, m_Pos.y, m_Size.x, m_Size.y);
    drawBlockTokens(GetPosInShape(), m_Tokens);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + SIDEBAR_ITEM_VSPACE);

    if (IsHovered()) {
        ImGui::SetTooltip("%s", m_Definition->description.c_str());
    }

    ImGui::PopID();
}

bool Block::IsHovered()
{
    ImVec2 topLeft = m_Pos;
    ImVec2 botRight = ImVec2(m_Pos.x + m_Size.x, m_Pos.y + m_Size.y);
    ImVec2 mousePos = ImGui::GetIO().MousePos;

    return mousePos.x >= topLeft.x && mousePos.x <= botRight.x
        && mousePos.y >= topLeft.y && mousePos.y <= botRight.y;
}

ImVec2 Block::GetPosInShape()
{
    ImVec2 textSize = ImGui::CalcTextSize(m_Definition->nameFmt.c_str());
    return ImVec2(m_Pos.x + BLOCK_HPAD, m_Pos.y + (BLOCK_HEIGHT - textSize.y) * 0.5);
}

void Block::UpdateSize()
{
    ImVec2 textSize = ImGui::CalcTextSize(m_Definition->nameFmt.c_str());
    m_Size = ImVec2(std::max(textSize.x + BLOCK_HPAD * 2.0f, BLOCK_MIN_WIDTH),
                    BLOCK_HEIGHT);
}

void drawBlockShape(float x, float y, float width, float height)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->PathLineTo(ImVec2(x, y));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET, y));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_SLOPE, y + BLOCK_NOTCH_HEIGHT));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH - BLOCK_NOTCH_SLOPE, y + BLOCK_NOTCH_HEIGHT));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH, y));
    drawList->PathLineTo(ImVec2(x + width, y));
    drawList->PathLineTo(ImVec2(x + width, y + height));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH, y + height));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_WIDTH - BLOCK_NOTCH_SLOPE, y + height + BLOCK_NOTCH_HEIGHT));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET + BLOCK_NOTCH_SLOPE, y + height + BLOCK_NOTCH_HEIGHT));
    drawList->PathLineTo(ImVec2(x + BLOCK_NOTCH_OFFSET, y + height));
    drawList->PathLineTo(ImVec2(x, y + height));
    drawList->PathLineTo(ImVec2(x, y));
    drawList->PathFillConcave(BLOCK_COLOR);
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
BlockInstance::BlockInstance(const Block &block, uint32_t id, bool isDragging)
    : Block(block.GetDefinition()),
      m_Id(id)
{
    m_Tokens = block.GetTokens();
    m_Pos = block.GetPos();
    m_Size = block.GetSize();
    m_IsDragging = isDragging;
}

BlockInstance::BlockInstance(const BlockInstance &instance, uint32_t id, const ImVec2 &pos)
    : BlockInstance(instance)
{
    m_Id = id;
    m_Pos = pos;
    m_IsDragging = false;
}

void BlockInstance::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;

    if (!m_IsDragging && IsHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_IsDragging = true;
        m_DragOffset = ImVec2(mousePos.x - m_Pos.x, mousePos.y - m_Pos.y);
        OnStartDrag();
    }

    if (m_IsDragging) {
        if (io.MouseDown[ImGuiMouseButton_Left]) {
            m_Pos.x = mousePos.x - m_DragOffset.x;
            m_Pos.y = mousePos.y - m_DragOffset.y;
        } else {
            m_IsDragging = false;
            OnEndDrag();
        }
    }
}

void BlockInstance::Draw()
{
    assert(m_Definition != nullptr);

    if (m_IsDragging) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::SetNextWindowPos(m_Pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(m_Size.x, m_Size.y + BLOCK_NOTCH_HEIGHT));

        ImGui::Begin("DraggingBlock", nullptr,
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_NoScrollWithMouse
                | ImGuiWindowFlags_NoInputs
                | ImGuiWindowFlags_NoDecoration);
    }

    ImGui::PushID(m_Id);

    UpdateSize();

    ImGui::SetCursorPos(m_Pos);
    drawBlockShape(m_Pos.x, m_Pos.y, m_Size.x, m_Size.y);
    drawBlockTokens(GetPosInShape(), m_Tokens);

    // Create an invisible button covering the whole block
    ImGui::SetCursorScreenPos(m_Pos);
    ImGui::InvisibleButton("BlockClickable", m_Size);

    if (ImGui::BeginPopupContextItem("Popup", ImGuiMouseButton_Right)) {
        if (ImGui::MenuItem("Delete")) OnDelete();
        if (ImGui::MenuItem("Duplicate")) OnDuplicate();
        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (m_IsDragging) {
        ImGui::End();
        ImGui::PopStyleVar();
    }
}


// Sidebar
// TODO: THIS IS SUPER UNSAFE
void Sidebar::Init()
{
    m_Blocks.reserve(g_BlockDefinitions.size());
    for (auto &definition : g_BlockDefinitions) {
        m_Blocks.emplace_back(&definition);
        Block &block = m_Blocks.back();
        block.OnStartDrag = [&]() { OnCreateBlock(block); };
    }
    LOG_DEBUG("initialized sidebar");
}

void Sidebar::Update()
{
    for (auto &block : m_Blocks) {
        block.Update();
    }
}

void Sidebar::Draw()
{
    ImGui::SetNextWindowSizeConstraints(
            ImVec2(SIDEBAR_MIN_WIDTH, 0.0f),
            ImVec2(SIDEBAR_MAX_WIDTH, FLT_MAX));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SIDEBAR_PAD, SIDEBAR_PAD));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, SIDEBAR_ITEM_VSPACE));
    ImGui::BeginChild(
            "Sidebar",
            ImVec2(SIDEBAR_MIN_WIDTH, 0.0f),
            ImGuiChildFlags_ResizeX | ImGuiChildFlags_AlwaysUseWindowPadding);

    for (auto &block : m_Blocks) {
        block.Draw();
    }

    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}


// Canvas
void Canvas::Init()
{
    LOG_DEBUG("Canvas Initialized");
}

void Canvas::Update()
{
    for (auto &block : m_Blocks) {
        block.Update();
    }
}

void Canvas::Draw()
{
    ImVec2 canvasSize(
            std::max(380.0f, ImGui::GetContentRegionAvail().x),
            std::max(260.0f, ImGui::GetContentRegionAvail().y));


    ImGui::BeginChild("CanvasFrame", canvasSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    for (auto &block : m_Blocks) {
        block.Draw();
    }
    ImGui::EndChild();

}

void Canvas::InstanceBlock(const Block &block)
{
    m_Blocks.emplace_back(block, m_NextId++, true);
    BlockInstance &instance = m_Blocks.back();
    SetInstanceCallbacks(instance);
    LOG_DEBUG("created instanced with id %u", instance.GetId());
    instance.OnStartDrag();
}

void Canvas::DuplicateInstance(const BlockInstance &original)
{
    m_Blocks.emplace_back(original, m_NextId++, ImVec2(original.GetPos().x + 100.0, original.GetPos().y + 100.0));
    BlockInstance &instance = m_Blocks.back();
    SetInstanceCallbacks(instance);

    LOG_DEBUG("duplicated instance with id %u", original.GetId());
}

void Canvas::DeleteInstance(uint32_t id)
{
    int32_t idx = -1;
    for (size_t i = 0; i < m_Blocks.size(); i ++) {
        if (id == m_Blocks[i].GetId()) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        LOG_ERROR("failed to find block with id %u", id);
        return;
    }

    m_Blocks.erase(m_Blocks.begin() + idx);
    LOG_DEBUG("deleted block with id %u", id);
}

void Canvas::BringToFront(uint32_t id)
{
    int32_t idx = -1;
    for (size_t i = 0; i < m_Blocks.size(); i ++) {
        if (id == m_Blocks[i].GetId()) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        LOG_ERROR("failed to find block with id %u", id);
        return;
    }

    std::swap(m_Blocks[idx], m_Blocks.back());
}

void Canvas::SetInstanceCallbacks(BlockInstance &instance)
{
    uint32_t id = m_Blocks.back().GetId();
    instance.OnDelete = [this, id] { DeleteInstance(id); };
    instance.OnDuplicate = [this, &instance] { DuplicateInstance(instance); };
    instance.OnStartDrag = [this, id] {
        BringToFront(id);
        m_IsDraggingBlock = true;
        LOG_DEBUG("drag started for instance %u", id);
    };
    instance.OnEndDrag = [this, id] {
        m_IsDraggingBlock = false;
        LOG_DEBUG("drag ended for instance %u", id);
    };
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

    m_Sidebar.OnCreateBlock = [&](const Block &block) {
        if (m_Canvas.IsDraggingBlock()) return;
        m_Canvas.InstanceBlock(block);
    };
}

void UI::Update()
{
    if (m_ShowSidebar) {
        m_Sidebar.Update();
    }
    m_Canvas.Update();
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

    if (m_ShowSidebar) m_Sidebar.Draw();
    ImGui::SameLine();
    m_Canvas.Draw();

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
