#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <unordered_set>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "style.hpp"
#include "build.hpp"

enum class BlockType
{
    Event = 0,
    Instruction,
    Expression,
};

enum ValueType : uint32_t
{
    Value_None = 0,

    Value_Int = 1 << 0,
    Value_Float = 1 << 1,
    Value_Bool = 1 << 2,
    Value_String = 1 << 3,

    Value_Number = Value_Int | Value_Float,
    Value_Any = 0xFFFFFFFF,
};

enum class BlockCategory
{
    Event = 0,
    Console,
    ControlFlow,
    Math,
    Logic,
    Variable,
};

struct CustomVariable
{
    std::string name;
    ValueType type = Value_Int;
};

struct BlockDefinition
{
    BlockType type;
    ValueType outputType = Value_None;
    std::string nameFmt;
    std::string description;
    BlockCategory category;

    std::string variableName = "";
    bool isVariableGetter = false;
    bool isVariableSetter = false;
    bool isReadIntoVariable = false;
};

enum class BlockTokenType
{
    Text,
    Input,
};

struct InputValue
{
    ValueType type = Value_None;
    std::string literal; // when no expression block attached
    uint32_t connectedBlockId = 0;
};

struct InputSocket
{
    ImVec2 pos;
    ImVec2 size;

    uint32_t ownerBlockId;
    uint32_t inputIndex;

    ValueType acceptedTypes;
};

struct BlockToken
{
    BlockTokenType type;

    // Text token
    std::string text;

    // Input token
    ValueType acceptedTypes = Value_None;
    std::string inputName;
    std::string defaultValue;
};

struct BlockData
{
    BlockDefinition definition;
    std::vector<BlockToken> tokens;

    explicit BlockData(const BlockDefinition *def);
};

enum class UIEventType
{
    BlockDragStarted,
    BlockDragged,
    BlockDragEnded,
    BlockInstanciateRequested,
    BlockDeleteReqested,
    BlockDuplicateReqested,
};

struct UIEvent
{
    UIEventType type;

    // data
    uint32_t id = 0;
    const BlockData *data = nullptr;
    ImVec2 delta = ImVec2(0, 0);
};

class UIEventQueue
{
    public:
        void Push(UIEvent event) { m_Events.push_back(event); }
        void Clear() { m_Events.clear(); }

        const std::vector<UIEvent> &Drain() { return m_Events; }

    private:
        std::vector<UIEvent> m_Events;
};

void DrawSidebarBlock(const BlockData &data, UIEventQueue &events);

struct BlockInstance
{
    uint32_t id;

    BlockData data;

    ImVec2 pos;
    ImVec2 size;

    std::vector<InputValue> inputs;

    // Conections
    uint32_t prevId = 0;
    uint32_t nextId = 0;

    // Inputs
    uint32_t parentBlockId = 0;
    int32_t parentInputIndex = -1;

    // UI state
    bool isDragging = false;
    bool isMouseDown = false;
    bool isMenuOpen = false;
    bool isActive = false;
    bool isHovered = false;

    ImVec2 mouseDownPos = ImVec2(0.0f, 0.0f);
    ImVec2 dragOffset = ImVec2(0.0f, 0.0f);
};

class Sidebar
{
    public:
        Sidebar() = default;
        ~Sidebar() = default;

        void Init();
        void Update();
        void Draw(UIEventQueue &events);

        const std::vector<CustomVariable> &GetVariables() const { return m_Variables; }

    private:
        void DrawAddVariablePopup();
        bool TryAddVariable(const std::string &name, ValueType type);
        void RebuildVariableBlocks();

        std::vector<BlockData> m_Blocks;
        std::vector<BlockData> m_VariableBlocks;
        std::vector<BlockDefinition> m_VariableDefinitions;
        std::vector<CustomVariable> m_Variables;

        bool m_ShowAddVariablePopup = false;
        char m_NewVarName[64] = {};
        int m_NewVarType = 0;
        const char *m_AddVariableError = nullptr;
};

enum class AttachType
{
    BottomNotch,
    TopNotch,
};

struct AttachTarget
{
    uint32_t id;
    ImVec2 pos;
    ImVec2 size;
    AttachType type = AttachType::BottomNotch;
};

class Canvas
{
    public:
        Canvas() = default;
        ~Canvas() = default;

        void Init();
        void Update(UIEventQueue &events);
        void Draw(UIEventQueue &events);

        void InstanceBlock(const BlockData &block);
        void DeleteInstance(uint32_t id);
        void BringToFront(uint32_t id);
        void DuplicateInstance(uint32_t id);
        void AttachInstance(uint32_t id, AttachTarget target);
        void DetachInstane(uint32_t id);

        void AttachExpressionBlock(uint32_t exprId, uint32_t parentId, uint32_t inputIndex);
        void DetachExpressionBlock(uint32_t exprId);
        void AppendBlockInputSockets(BlockInstance &block);
        void RebuildInputSockets();

        void WalkBlockSequence(uint32_t id, std::function<void (BlockInstance &inst)> callback);
        void MoveAttachedExpression(BlockInstance &parent);

        void CollectBlockTree(uint32_t id, std::unordered_set<uint32_t>& visited);

        std::optional<BlockInstance> GetMainBlock();

        ImVec2 WorldToScreen(ImVec2 pos);
        ImVec2 ScreenToWorld(ImVec2 pos);

        std::optional<AttachTarget> FindAttachTarget(const BlockInstance &instance);
        std::optional<InputSocket> FindInputSocketTarget(const BlockInstance &instance);
        std::vector<BlockInstance>::iterator FindBlockById(uint32_t id);
        int32_t FindIdxById(uint32_t id);

        std::vector<BlockInstance> &GetBlocks() { return m_Blocks; };

        bool IsDraggingBlock = false;

    private:
        std::vector<BlockInstance> m_Blocks;
        uint32_t m_NextId = 1;

        std::vector<InputSocket> m_InputSockets;

        bool m_IsPanning = false;
        ImVec2 m_PanOffset = ImVec2(0.0, 0.0);
        ImVec2 m_LastMousePos = ImVec2(0.0, 0.0);
};

void DrawCanvasBlock(Canvas &canvas, BlockInstance &Block, UIEventQueue &events);
void UpdateCanvasBlock(Canvas &canvas, BlockInstance &block, UIEventQueue &events);

#include "codegen.hpp"

class CodeView
{
    public:
        CodeView() = default;
        ~CodeView() = default;

        void Generate(Canvas &canvas, const std::vector<CustomVariable> &variables);
        void Draw();

    const std::string &GetCode() const { return m_Code; }

    private:
        std::string m_Code;
};

class UI
{
    public:
        UI() = default;
        ~UI() = default;

        void Init();
        void Update();
        void Draw();
        void Destroy();

        GLFWwindow *GetWindow() { return m_Window; }

    private:
        void StartFrame();
        void Render();

        void DrawMainMenuBar();
        void DrawWorkspace();
        void DrawOutputPanel();

    private:
        GLFWwindow *m_Window = nullptr;

        UIEventQueue m_EventQueue;

        Sidebar m_Sidebar;
        Canvas m_Canvas;
        CodeView m_CodeView;

        bool m_ShowSidebar = true;
        bool m_ShowOutputPanel = false;
        bool m_ShowCodeView = false;

    BuildSettings m_BuildSettings;
    BuildResult m_LastBuild;
    RunResult m_LastRun;

    bool m_HasBuilt = false;
    bool m_HasRan = false;
};

inline const std::vector<BlockDefinition> g_BlockDefinitions = {
    BlockDefinition { BlockType::Event,       Value_None,   "Main",                                      "Main entry point",                                        BlockCategory::Event       },
    BlockDefinition { BlockType::Instruction, Value_None,   "Write {any:text=Hello World}",              "Writes to the console",                                   BlockCategory::Console     },
    BlockDefinition { BlockType::Instruction, Value_None,   "Read into {string:var=}",                   "Reads input from the console into a variable",            BlockCategory::Console,     "", false, false, true },
    BlockDefinition { BlockType::Instruction, Value_None,   "End Line",                                  "Moves to the next line",                                  BlockCategory::Console     },
    BlockDefinition { BlockType::Instruction, Value_None,   "If {bool:val=false} then",                  "Checks for condition",                                    BlockCategory::ControlFlow },
    BlockDefinition { BlockType::Instruction, Value_None,   "Else",                                      "Else",                                                    BlockCategory::ControlFlow },
    BlockDefinition { BlockType::Instruction, Value_None,   "End If",                                    "Ends if",                                                 BlockCategory::ControlFlow },
    BlockDefinition { BlockType::Expression,  Value_Number, "{number:left=1} + {number:right=1}",        "Adds 2 numbers",                                          BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Number, "{number:left=1} - {number:right=1}",        "Subtracts 2 numbers",                                     BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Number, "{number:left=1} * {number:right=1}",        "Multiplies 2 numbers",                                    BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Number, "{number:left=1} / {number:right=1}",        "Divies 2 numbers",                                        BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Number, "{int:left=1} mod {int:right=1}",            "Adds 2 numbers",                                          BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Float,  "round {number:value=0.5}",                  "Rounds a number",                                         BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Float,  "abs {number:value=0.5}",                    "Absolute of a number",                                    BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Float,  "sqrt {number:value=0.5}",                   "Square root of a number",                                 BlockCategory::Math        },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "true",                                      "true value",                                              BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "false",                                     "false value",                                             BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{number:left=0.5} < {number:right=0.5}",    "Checks if a number is less than another number",          BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{number:left=0.5} <= {number:right=0.5}",   "Checks if a number is less or equal than another number", BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{number:left=0.5} > {number:right=0.5}",    "Checks if a number is greater than another number",       BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{number:left=0.5} >= {number:right=0.5}",   "Checks if a number is greater or equal and another",      BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{number:left=0.5} = {number:right=0.5}",    "Checks if 2 numbers are equal",                           BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "not {bool:value=true}",                     "Negates a condition",                                     BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{bool:value=true} and {bool:value=false}",  "Ands 2 conditions",                                       BlockCategory::Logic       },
    BlockDefinition { BlockType::Expression,  Value_Bool,   "{bool:value=true} or {bool:value=false}",   "Ors 2 conditions",                                        BlockCategory::Logic       },
};
