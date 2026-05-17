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
        ImVec2 m_DragOffset;
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
        void DuplicateInstance(const BlockInstance &original);

    private:
        void SetInstanceCallbacks(BlockInstance &instance);

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

        Sidebar m_Sidebar;
        Canvas m_Canvas;

        bool m_ShowSidebar = true;
        bool m_ShowOutputPanel = false;
};

inline const std::vector<BlockDefinition> g_BlockDefinitions = {
    BlockDefinition { BlockType::Instruction, "Write {str=Hello World}", "Writes to the console" },
    BlockDefinition { BlockType::Instruction, "Add {int=1} and {int=1}", "Adds 2 numbers" },
    BlockDefinition { BlockType::Instruction, "Round {float=0.5}", "Rounds a number" },
    BlockDefinition { BlockType::Instruction, "Clear", "Clears the console" },
};
