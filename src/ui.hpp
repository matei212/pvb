#pragma once

#include <vector>
#include <string>
#include <functional>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "style.hpp"

enum class BlockType
{
    Event = 0,
    Instruction,
    Expression,
};

enum class BlockCategory
{
    Event = 0,
    Console,
    Math,
    Logic,
};

struct BlockDefinition
{
    BlockType type;
    std::string nameFmt;
    std::string description;
    BlockCategory category;
};

enum class BlockTokenType
{
    Text,
    StringInput,
    IntInput,
    FloatInput,
};

struct BlockToken
{
    BlockTokenType type;
    std::string text;

    std::string strValue;
    int intValue;
    float floatValue;
};

struct BlockData
{
    const BlockDefinition *definition;
    std::vector<BlockToken> tokens;

    explicit BlockData(const BlockDefinition *def);
};

enum class UIEventType
{
    BlockDragStarted,
    BlockDragEnded,
    BlockInstanciateRequested,
    BlockDeleteReqested,
    BlockDuplicateReqested,
};

struct UIEvent
{
    UIEventType type;

    // data
    uint32_t id;
    const BlockData *data;
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

    // Input state
    bool isDragging = false;
    bool isMouseDown = false;
    bool isMenuOpen = false;
    bool isActive = false;
    ImVec2 mouseDownPos = ImVec2(0.0f, 0.0f);
    ImVec2 dragOffset = ImVec2(0.0f, 0.0f);
};

void DrawCanvasBlock(BlockInstance &Block, UIEventQueue &events);
void UpdateCanvasBlock(BlockInstance &block, UIEventQueue &events);

class Sidebar
{
    public:
        Sidebar() = default;
        ~Sidebar() = default;

        void Init();
        void Update();
        void Draw(UIEventQueue &events);

    private:
        std::vector <BlockData> m_Blocks;
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

        std::vector<BlockInstance>::iterator FindBlockById(uint32_t id);
        int32_t FindIdxById(uint32_t id);

        bool IsDraggingBlock = false;

    private:
        std::vector<BlockInstance> m_Blocks;
        uint32_t m_NextId = 1;
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

        bool m_ShowSidebar = true;
        bool m_ShowOutputPanel = false;
};

inline const std::vector<BlockDefinition> g_BlockDefinitions = {
    BlockDefinition { BlockType::Event,       "Main",                       "Main entry point",                                        BlockCategory::Event   },
    BlockDefinition { BlockType::Instruction, "Write {str=Hello World}",    "Writes to the console",                                   BlockCategory::Console },
    BlockDefinition { BlockType::Instruction, "Clear",                      "Clears the console",                                      BlockCategory::Console },
    BlockDefinition { BlockType::Expression,  "{float=1} + {float=1}",      "Adds 2 numbers",                                          BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "{float=1} - {float=1}",      "Subtracts 2 numbers",                                     BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "{float=1} * {float=1}",      "Multiplies 2 numbers",                                    BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "{float=1} / {float=1}",      "Divies 2 numbers",                                        BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "{int=1} mod {int=1}",        "Adds 2 numbers",                                          BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "round {float=0.5}",          "Rounds a number",                                         BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "abs {float=0.5}",            "Absolute of a number",                                    BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "sqrt {float=0.5}",           "Square root of a number",                                 BlockCategory::Math    },
    BlockDefinition { BlockType::Expression,  "true",                       "true value",                                              BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "false",                      "false value",                                             BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0.5} < {float=0.5}",  "Checks if a number is less than another number",          BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0.5} <= {float=0.5}", "Checks if a number is less or equal than another number", BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0.5} > {float=0.5}",  "Checks if a number is greater than another number",       BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0.5} >= {float=0.5}", "Checks if a number is greater or equal and another",      BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0.5} = {float=0.5}",  "Checks if 2 numbers are equal",                           BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "not {float=0}",              "Negates a condition",                                     BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0} and {float=0}",    "Ands 2 conditions",                                       BlockCategory::Logic   },
    BlockDefinition { BlockType::Expression,  "{float=0} or {float=0}",     "Ors 2 conditions",                                        BlockCategory::Logic   },
};
