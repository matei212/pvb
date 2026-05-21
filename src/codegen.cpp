#include "codegen.hpp"

std::unique_ptr<ASTNode> ASTNode::make(ASTNodeKind k)
{
    auto n   = std::make_unique<ASTNode>();
    n->kind  = k;
    return n;
}

std::unique_ptr<ASTNode> ASTNode::makeLiteral(const std::string &s)
{
    auto n  = make(ASTNodeKind::Literal);
    n->sval = s;
    return n;
}

std::unique_ptr<ASTNode> ASTNode::makeBinOp(const std::string &op,
        std::unique_ptr<ASTNode> l,
        std::unique_ptr<ASTNode> r)
{
    auto n  = make(ASTNodeKind::BinOp);
    n->sval = op;
    n->children.push_back(std::move(l));
    n->children.push_back(std::move(r));
    return n;
}

std::unique_ptr<ASTNode> ASTNode::makeUnaryOp(const std::string &op, std::unique_ptr<ASTNode> operand)
{
    auto n  = make(ASTNodeKind::UnaryOp);
    n->sval = op;
    n->children.push_back(std::move(operand));
    return n;
}

std::unique_ptr<ASTNode> ASTBuilder::build(const BlockInstance &eventBlock, Canvas &canvas)
{
    if (eventBlock.data.definition.type != BlockType::Event) {
        LOG_ERROR("ASTBuilder::build() requires an Event block");
        return nullptr;
    }

    auto program  = ASTNode::make(ASTNodeKind::Program);
    auto function = ASTNode::make(ASTNodeKind::Function);
    function->sval = "main"; // event name (extend if you have named events)

    uint32_t nextId = eventBlock.nextId;
    buildStatementChain(nextId, canvas, *function);

    program->children.push_back(std::move(function));
    return program;
}

uint32_t ASTBuilder::buildStatementChain(uint32_t startId, Canvas &canvas, ASTNode &parent)
{
    uint32_t current = startId;

    while (current != 0) {
        auto it = canvas.FindBlockById(current);
        if (it == canvas.GetBlocks().end()) {
            LOG_ERROR("dangling block id %s", std::to_string(current).c_str());
            return static_cast<uint32_t>(-1);
        }

        const BlockInstance &block = *it;
        const std::string   &fmt  = block.data.definition.nameFmt;

        if (fmt == "End If" || fmt == "Else")
            return current;

        auto stmt = buildStatement(block, canvas);
        if (!stmt) {
            current = block.nextId;
            continue;
        }

        if (stmt->kind == ASTNodeKind::If) {
            current = block.nextId;
            current = buildIfBody(current, canvas, *stmt);
            parent.children.push_back(std::move(stmt));
            continue;
        }

        parent.children.push_back(std::move(stmt));
        current = block.nextId;
    }

    return 0;
}

uint32_t ASTBuilder::buildIfBody(uint32_t startId, Canvas &canvas, ASTNode &ifNode)
{
    uint32_t cursor = buildStatementChain(startId, canvas, ifNode);

    if (cursor == 0) {
        LOG_ERROR("If block missing End If");
        return static_cast<uint32_t>(-1);
    }

    auto it  = canvas.FindBlockById(cursor);
    const std::string &fmt = it->data.definition.nameFmt;

    if (fmt == "Else") {
        auto elseNode = ASTNode::make(ASTNodeKind::Else);
        cursor = it->nextId;
        cursor = buildStatementChain(cursor, canvas, *elseNode);
        ifNode.children.push_back(std::move(elseNode));

        if (cursor == 0) {
            LOG_ERROR("If block missing End If after Else");
            return static_cast<uint32_t>(-1);
        }

        it  = canvas.FindBlockById(cursor);
    }

    return it->nextId;
}

std::unique_ptr<ASTNode> ASTBuilder::buildStatement(const BlockInstance &block, Canvas &canvas)
{
    const std::string &fmt = block.data.definition.nameFmt;

    if (fmt.rfind("Write ", 0) == 0 || fmt == "Write {any:text=Hello World}") {
        auto node  = ASTNode::make(ASTNodeKind::WriteLn);
        node->children.push_back(buildInputExpr(block, canvas, 0));
        return node;
    }

    if (fmt == "End Line")
        return ASTNode::make(ASTNodeKind::EndLine);

    if (block.data.definition.isReadIntoVariable) {
        auto node = ASTNode::make(ASTNodeKind::ReadVar);
        if (!block.inputs.empty())
            node->sval = block.inputs[0].literal;
        return node;
    }

    if (fmt.rfind("If ", 0) == 0) {
        auto node = ASTNode::make(ASTNodeKind::If);
        node->children.push_back(buildInputExpr(block, canvas, 0));
        return node;
    }

    if (block.data.definition.isVariableSetter) {
        auto node = ASTNode::make(ASTNodeKind::Assign);
        node->sval = block.data.definition.variableName;
        node->children.push_back(buildInputExpr(block, canvas, 0));
        return node;
    }

    return nullptr;
}

std::unique_ptr<ASTNode> ASTBuilder::buildInputExpr(const BlockInstance &block, Canvas &canvas, size_t inputIndex)
{
    if (inputIndex >= block.inputs.size())
        return ASTNode::makeLiteral("0");

    const InputValue &input = block.inputs[inputIndex];

    if (input.connectedBlockId != 0) {
        auto it = canvas.FindBlockById(input.connectedBlockId);
        if (it == canvas.GetBlocks().end())
            return ASTNode::makeLiteral("0");

        return buildExprBlock(*it, canvas);
    }

    return ASTNode::makeLiteral(input.literal);
}

std::unique_ptr<ASTNode> ASTBuilder::buildExprBlock(const BlockInstance &block, Canvas &canvas)
{
    const std::string &fmt = block.data.definition.nameFmt;

    if (block.data.definition.isVariableGetter) {
        auto node  = ASTNode::make(ASTNodeKind::VarRef);
        node->sval = block.data.definition.variableName;
        return node;
    }

    static const std::vector<std::pair<std::string,std::string>> binOps = {
        { "{number:left=1} + {number:right=1}", "+"   },
        { "{number:left=1} - {number:right=1}", "-"   },
        { "{number:left=1} * {number:right=1}", "*"   },
        { "{number:left=1} / {number:right=1}", "/"   },
        { "{int:left=1} mod {int:right=1}",     "%"   },
        { "{number:left=0.5} < {number:right=0.5}",  "<"  },
        { "{number:left=0.5} <= {number:right=0.5}", "<=" },
        { "{number:left=0.5} > {number:right=0.5}",  ">"  },
        { "{number:left=0.5} >= {number:right=0.5}", ">=" },
        { "{number:left=0.5} = {number:right=0.5}",  "==" },
        { "{bool:value=true} and {bool:value=false}",   "&&" },
        { "{bool:value=true} or {bool:value=false}",    "||" },
    };

    for (auto &[pattern, op] : binOps) {
        if (fmt == pattern) {
            return ASTNode::makeBinOp(
                    op,
                    buildInputExpr(block, canvas, 0),
                    buildInputExpr(block, canvas, 1));
        }
    }

    static const std::vector<std::pair<std::string,std::string>> unaryOps = {
        { "round {number:value=0.5}", "round" },
        { "abs {number:value=0.5}",  "abs"   },
        { "sqrt {number:value=0.5}", "sqrt"  },
        { "not {bool:value=true}",     "!"     },
    };

    for (auto &[pattern, op] : unaryOps) {
        if (fmt == pattern) {
            return ASTNode::makeUnaryOp(
                    op,
                    buildInputExpr(block, canvas, 0));
        }
    }

    if (fmt == "true") {
        auto n  = ASTNode::makeLiteral("true");
        return n;
    }
    if (fmt == "false") {
        auto n  = ASTNode::makeLiteral("false");
        return n;
    }

    // Fallback: treat the block's first literal input as a raw value
    if (!block.inputs.empty())
        return ASTNode::makeLiteral(block.inputs[0].literal);

    return ASTNode::makeLiteral("0");
}

std::string CodeGen::emit(const ASTNode &root, const std::vector<CustomVariable> &variables)
{
    m_Out.clear();
    m_Indent = 0;
    m_Variables = variables;

    if (root.kind != ASTNodeKind::Program) {
        LOG_ERROR("CodeGen::emit() expects an ASTNodeKind::Program root");
        return "";
    }

    // Standard headers
    line("#include <iostream>");
    line("#include <cmath>");
    line("#include <string>");
    line("using namespace std;");
    line("");

    emitNode(root);

    return m_Out;
}

void CodeGen::emitNode(const ASTNode &node)
{
    switch (node.kind)
    {
        case ASTNodeKind::Program:   emitProgram(node);  break;
        case ASTNodeKind::Function:  emitFunction(node); break;
        case ASTNodeKind::WriteLn:   emitWrite(node);    break;
        case ASTNodeKind::EndLine:   emitEndLine(node);  break;
        case ASTNodeKind::ReadVar:   emitReadVar(node);  break;
        case ASTNodeKind::If:        emitIf(node);       break;
        case ASTNodeKind::Else:      emitElse(node);     break;
        case ASTNodeKind::Literal:   emitLiteral(node);  break;
        case ASTNodeKind::VarRef:    emitVarRef(node);   break;
        case ASTNodeKind::Assign:    emitAssign(node);   break;
        case ASTNodeKind::BinOp:     emitBinOp(node);    break;
        case ASTNodeKind::UnaryOp:   emitUnaryOp(node);  break;
        default:
                                     LOG_ERROR("CodeGen: unknown ASTNodeKind");
                                     break;
    }
}

void CodeGen::emitProgram(const ASTNode &node)
{
    for (auto &child : node.children)
        emitNode(*child);
}
 
void CodeGen::emitFunction(const ASTNode &node)
{
    line("int " + node.sval + "()");
    line("{");
    indent();

    for (const auto &var : m_Variables) {
        if (var.type == Value_Int)
            line("int " + var.name + " = 0;");
        else if (var.type == Value_Float)
            line("float " + var.name + " = 0.0f;");
        else if (var.type & Value_String)
            line("string " + var.name + " = \"\";");
    }

    if (!m_Variables.empty())
        line("");

    for (auto &child : node.children)
        emitNode(*child);

    line("return 0;");
    dedent();
    line("}");
}

void CodeGen::emitWrite(const ASTNode &node)
{
    if (node.children.empty()) {
        LOG_ERROR("Write node has no children");
        return;
    }

    // Begin the line with the indentation + "cout << "
    for (int i = 0; i < m_Indent; ++i)
        m_Out += "    ";

    m_Out += "cout << ";
    emitExpr(*node.children[0]);
    m_Out += ";\n";
}
 
void CodeGen::emitEndLine(const ASTNode &)
{
    line("cout << endl;");
}

void CodeGen::emitReadVar(const ASTNode &node)
{
    if (node.sval.empty()) {
        LOG_ERROR("Read into variable block has no target variable");
        return;
    }

    line("cin >> " + node.sval + ";");
}

void CodeGen::emitIf(const ASTNode &node)
{
    if (node.children.empty()) {
        LOG_ERROR("If node has no condition");
        return;
    }

    for (int i = 0; i < m_Indent; ++i)
        m_Out += "    ";
    m_Out += "if (";
    emitExpr(*node.children[0]);
    m_Out += ")\n";

    line("{");
    indent();

    for (size_t i = 1; i < node.children.size(); ++i) {
        const ASTNode &child = *node.children[i];
        if (child.kind == ASTNodeKind::Else) {
            dedent();
            line("}");
            emitElse(child);
            goto done;
        }
        emitNode(child);
    }

    dedent();
    line("}");

done:;
}

void CodeGen::emitElse(const ASTNode &node)
{
    line("else");
    line("{");
    indent();
    for (auto &child : node.children)
        emitNode(*child);
    dedent();
    line("}");
}

void CodeGen::emitExpr(const ASTNode &node)
{
    switch (node.kind)
    {
        case ASTNodeKind::Literal:  emitLiteral(node);  break;
        case ASTNodeKind::VarRef:   emitVarRef(node);   break;
        case ASTNodeKind::BinOp:    emitBinOp(node);    break;
        case ASTNodeKind::UnaryOp:  emitUnaryOp(node);  break;
        default:
                                    LOG_ERROR("CodeGen::emitExpr: not an expression node");
                                    break;
    }
}

void CodeGen::emitVarRef(const ASTNode &node)
{
    m_Out += node.sval;
}

void CodeGen::emitAssign(const ASTNode &node)
{
    if (node.children.empty()) {
        LOG_ERROR("Assign node has no value");
        return;
    }

    for (int i = 0; i < m_Indent; ++i)
        m_Out += "    ";

    m_Out += node.sval + " = ";
    emitExpr(*node.children[0]);
    m_Out += ";\n";
}

void CodeGen::emitLiteral(const ASTNode &node)
{
    const std::string &s = node.sval;

    if (s.empty()) { m_Out += "0"; return; }

    // Already a bool keyword
    if (s == "true" || s == "false") { m_Out += s; return; }

    // Check if it's a number (int or float)
    bool isNumber = true;
    bool hasDot   = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '-' && i == 0) continue;
        if (c == '.') { hasDot = true; continue; }
        if (!std::isdigit(static_cast<unsigned char>(c))) { isNumber = false; break; }
    }

    if (isNumber) {
        m_Out += s;
        if (hasDot) m_Out += "f"; // make it a float literal explicitly
        return;
    }

    // Treat as a string literal
    m_Out += '"';
    for (char c : s)
    {
        if (c == '"')  { m_Out += "\\\""; continue; }
        if (c == '\\') { m_Out += "\\\\"; continue; }
        if (c == '\n') { m_Out += "\\n";  continue; }
        m_Out += c;
    }
    m_Out += '"';
}

void CodeGen::emitBinOp(const ASTNode &node)
{
    if (node.children.size() < 2) {
        LOG_ERROR("BinOp node needs 2 children");
        return;
    }

    m_Out += "(";
    emitExpr(*node.children[0]);
    m_Out += " " + node.sval + " ";
    emitExpr(*node.children[1]);
    m_Out += ")";
}

void CodeGen::emitUnaryOp(const ASTNode &node)
{
    if (node.children.empty()) {
        LOG_ERROR("UnaryOp node needs 1 child");
        return;
    }

    const std::string &op = node.sval;


    if (op == "!")
    {
        m_Out += "!(";
        emitExpr(*node.children[0]);
        m_Out += ")";
        return;
    }


    m_Out += op + "(";
    emitExpr(*node.children[0]);
    m_Out += ")";
}

void CodeGen::indent()  { ++m_Indent; }
void CodeGen::dedent()  { if (m_Indent > 0) --m_Indent; }

void CodeGen::line(const std::string &s)
{
    for (int i = 0; i < m_Indent; ++i)
        m_Out += "    ";
    m_Out += s;
    m_Out += '\n';
}

void CodeGen::append(const std::string &s)
{
    m_Out += s;
}
