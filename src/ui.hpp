#pragma once

#include <vector>
#include <string>

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

        void Update();
        void Draw();
        bool IsHovered();

    private:
        ImVec2 m_Pos;
        ImVec2 m_Size;

        const BlockDefinition *m_Definition;
        std::vector<BlockToken> m_Tokens;

};

class Sidebar
{
    public:
        Sidebar() = default;
        ~Sidebar() = default;

        void Init();
        void Update();
        void Draw();

    private:
        std::vector<Block> m_Blocks;
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
        void DrawCanvas();
        void DrawOutputPanel();

    private:
        GLFWwindow *m_Window = nullptr;

        Sidebar m_Sidebar;
        bool m_ShowSidebar = true;
        bool m_ShowOutputPanel = false;
};

inline const std::vector<BlockDefinition> g_BlockDefinitions = {
    BlockDefinition { BlockType::Instruction, "Write {str=Hello World}", "Writes to the console" },
    BlockDefinition { BlockType::Instruction, "Add {int=1} and {int=1}", "Adds 2 numbers" },
    BlockDefinition { BlockType::Instruction, "Round {float=0.5}", "Rounds a number" },
    BlockDefinition { BlockType::Instruction, "Clear", "Clears the console" },
};
