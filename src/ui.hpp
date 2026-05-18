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

struct BlockDefinition
{
    BlockType type;
    std::string nameFmt;
    std::string description;
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

class Block
{
    public:
        Block(const BlockDefinition* definition);
        ~Block() = default;

        virtual void Update();
        virtual void Draw();

        bool IsHovered();

        const BlockDefinition *GetDefinition() const { return m_Definition; }
        const std::vector<BlockToken> &GetTokens() const { return m_Tokens; }
        const ImVec2 &GetPos() const { return m_Pos; }
        const ImVec2 &GetSize() const { return m_Size; }

        std::function<void ()> OnStartDrag;
        std::function<void ()> OnEndDrag;

    protected:
        ImVec2 GetPosInShape();
        void UpdateSize();

    protected:
        ImVec2 m_Pos;
        ImVec2 m_Size;
        bool m_IsDragging = false;

        const BlockDefinition *m_Definition;
        std::vector<BlockToken> m_Tokens;
};

class BlockInstance : public Block
{
    public:
        BlockInstance(const BlockInstance &instance) = default;
        ~BlockInstance() = default;

        BlockInstance(const BlockDefinition *definition) : Block(definition) {}
        BlockInstance(const Block &block, uint32_t id, bool isDragging = false);
        BlockInstance(const BlockInstance &instance, uint32_t id, const ImVec2 &pos);

        void Update() override;
        void Draw() override;

        uint32_t GetId() const { return m_Id; };

        std::function<void ()> OnDelete;
        std::function<void ()> OnDuplicate;

    private:
        uint32_t m_Id;

        ImVec2 m_MouseDownPos;
        bool m_IsMouseDown;
        ImVec2 m_DragOffset;

        bool m_IsMenuOpen = false;
        bool m_IsHovered = false;
        bool m_IsActive = false;
};

class Sidebar
{
    public:
        Sidebar() = default;
        ~Sidebar() = default;

        void Init();
        void Update();
        void Draw();

        std::function<void(const Block &block)> OnCreateBlock;

    private:
        std::vector<Block> m_Blocks;
};

class Canvas
{
    public:
        Canvas() = default;
        ~Canvas() = default;

        void Init();
        void Update();
        void Draw();

        void InstanceBlock(const Block &block);
        void DeleteInstance(uint32_t id);
        void BringToFront(uint32_t id);
        void DuplicateInstance(uint32_t id);

        bool IsDraggingBlock() { return m_IsDraggingBlock; }

    private:
        void SetInstanceCallbacks(BlockInstance &instance);

    private:
        std::vector<BlockInstance> m_Blocks;
        uint32_t m_NextId = 1;

        bool m_IsDraggingBlock = false;
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

        Sidebar m_Sidebar;
        Canvas m_Canvas;

        bool m_ShowSidebar = true;
        bool m_ShowOutputPanel = false;
};

inline const std::vector<BlockDefinition> g_BlockDefinitions = {
    BlockDefinition { BlockType::Event, "Main", "Main entry point" },
    BlockDefinition { BlockType::Instruction, "Write {str=Hello World}", "Writes to the console" },
    BlockDefinition { BlockType::Instruction, "Clear", "Clears the console" },
    BlockDefinition { BlockType::Expression, "{float=1} + {float=1}", "Adds 2 numbers" },
    BlockDefinition { BlockType::Expression, "{float=1} - {float=1}", "Subtracts 2 numbers" },
    BlockDefinition { BlockType::Expression, "{float=1} * {float=1}", "Multiplies 2 numbers" },
    BlockDefinition { BlockType::Expression, "{float=1} / {float=1}", "Divies 2 numbers" },
    BlockDefinition { BlockType::Expression, "{float=1} mod {int=1}", "Adds 2 numbers" },
    BlockDefinition { BlockType::Expression, "Round {float=0.5}", "Rounds a number" },
    BlockDefinition { BlockType::Expression, "Abs {float=0.5}", "Absolute of a number" },
    BlockDefinition { BlockType::Expression, "Sqrt {float=0.5}", "Square root of a number" },
};
