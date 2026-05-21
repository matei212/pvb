#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ui.hpp"
#include "log.hpp"

// Ast
enum class ASTNodeKind
{
    Program,       // root wrapper
    Function,      // event block

    WriteLn,       // "Write"  instruction
    EndLine,       // "End Line" instruction
    ReadVar,       // "Read into" instruction
    If,            // "If <cond> then" … "End If"
    Else,          // "Else" marker (child of If)

    Literal,       // int / float / bool / string literal
    VarRef,        // reference to a custom variable
    Assign,        // set variable to a value
    BinOp,         // +  -  *  /  mod  <  <=  >  >=  =  and  or
    UnaryOp,       // not   round   abs   sqrt
};

struct ASTNode
{
    ASTNodeKind kind;

    std::string sval;   // string value or operator symbol
    double      nval = 0.0;
    bool        bval = false;

    std::vector<std::unique_ptr<ASTNode>> children;

    // Convenience constructors
    static std::unique_ptr<ASTNode> make(ASTNodeKind k);
    static std::unique_ptr<ASTNode> makeLiteral(const std::string &s);
    static std::unique_ptr<ASTNode> makeBinOp(const std::string &op,
                                               std::unique_ptr<ASTNode> l,
                                               std::unique_ptr<ASTNode> r);
    static std::unique_ptr<ASTNode> makeUnaryOp(const std::string &op,
                                                 std::unique_ptr<ASTNode> operand);
};

class ASTBuilder
{
public:
    std::unique_ptr<ASTNode> build(const BlockInstance &eventBlock, Canvas &canvas);

private:
    uint32_t buildStatementChain(uint32_t startId, Canvas &canvas, ASTNode &parent);
    uint32_t buildIfBody(uint32_t startId, Canvas &canvas, ASTNode &ifNode);
    std::unique_ptr<ASTNode> buildStatement(const BlockInstance &block, Canvas &canvas);
    std::unique_ptr<ASTNode> buildInputExpr(const BlockInstance &block, Canvas &canvas, size_t inputIndex);
    std::unique_ptr<ASTNode> buildExprBlock(const BlockInstance &block, Canvas &canvas);
};

class CodeGen
{
public:
    std::string emit(const ASTNode &root, const std::vector<CustomVariable> &variables);

private:
    void emitNode(const ASTNode &node);

    void emitProgram (const ASTNode &node);
    void emitFunction(const ASTNode &node);
    void emitWrite   (const ASTNode &node);
    void emitEndLine (const ASTNode &node);
    void emitReadVar (const ASTNode &node);
    void emitIf      (const ASTNode &node);
    void emitElse    (const ASTNode &node);
    void emitExpr    (const ASTNode &node);
    void emitLiteral (const ASTNode &node);
    void emitVarRef  (const ASTNode &node);
    void emitAssign  (const ASTNode &node);
    void emitBinOp   (const ASTNode &node);
    void emitUnaryOp (const ASTNode &node);

    void indent();
    void dedent();
    void line(const std::string &s);
    void append(const std::string &s);

    std::string m_Out;
    int         m_Indent = 0;
    std::vector<CustomVariable> m_Variables;
};
