#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"



std::string typeToString(const Type& t) {
    std::string base;
    switch (t.base) {
        case TYPE_INT: base = "int"; break;
        case TYPE_CHAR: base = "char"; break;
        case TYPE_BOOL: base = "bool"; break;
        case TYPE_FLOAT: base = "float"; break;
        case TYPE_VOID: base = "void"; break;
        default: base = "unknown";
    }
    if (t.isArray) {
        base += "[" + std::to_string(t.size) + "]";
    }
    return base;
}

void printCommand(const Command& cmd) {
    switch (cmd.inst) {
        case ADD: std::cout << "ADD"; break;
        case SUB: std::cout << "SUB"; break;
        case MUL: std::cout << "MUL"; break;
        case DIV: std::cout << "DIV"; break;
        case MOD: std::cout << "MOD"; break;
        case NEG: std::cout << "NEG"; break;
        case GREATER: std::cout << "GREATER"; break;
        case LESS: std::cout << "LESS"; break;
        case GE: std::cout << "GE"; break;
        case LE: std::cout << "LE"; break;
        case EQ: std::cout << "EQ"; break;
        case NE: std::cout << "NE"; break;
        case AND: std::cout << "AND"; break;
        case OR: std::cout << "OR"; break;
        case NOT: std::cout << "NOT"; break;
        case POP: std::cout << "POP"; break;
        case PUSH:
            std::cout << "PUSH ";
            if (!cmd.argName.empty())
                std::cout << cmd.argName;
            else
                std::cout << cmd.intArg;
            break;
        case LOAD:
            std::cout << "LOAD " << cmd.argName;
            break;
        case LOAD_ARRAY:
            std::cout << "LOAD_ARRAY " << cmd.argName;
            break;
        case STORE_ARRAY:
            std::cout << "STORE_ARRAY " << cmd.argName;
            break;
        case JUMP:
            std::cout << "JUMP " << cmd.intArg;
            break;
        case JUMPIFTRUE:
            std::cout << "JUMPIFTRUE " << cmd.intArg;
            break;
        case JUMPIFFALSE:
            std::cout << "JUMPIFFALSE " << cmd.intArg;
            break;
        case SCOPEOPEN:
            std::cout << "SCOPEOPEN";
            break;
        case DECLAREVAR:
            std::cout << "DECLAREVAR " << cmd.argName << " " << typeToString(cmd.argType);
            break;
        case SCOPEPOP:
            std::cout << "SCOPEPOP";
            break;
        case CALL:
            std::cout << "CALL " << cmd.argName;
            break;
        case RET:
            std::cout << "RET";
            break;
        case ASSIGN:
            std::cout << "ASSIGN " << cmd.argName;
            break;
        default:
            std::cout << "UNKNOWN";
    }
    std::cout << std::endl;
}

void printAST(ASTNode* node, int indent = 0) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) std::cout << "  ";

    if (Program* p = dynamic_cast<Program*>(node)) {
        std::cout << "Program\n";
        for (size_t i = 0; i < p->declarations.size(); ++i)
            printAST(p->declarations[i], indent + 1);
    } else if (FunctionDecl* f = dynamic_cast<FunctionDecl*>(node)) {
        std::cout << "Function " << f->name << " (";
        for (size_t i = 0; i < f->params.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << f->params[i].name << ": ";
            // вывод типа (упрощённо)
            if (f->params[i].type.isArray) std::cout << "array";
            else if (f->params[i].type.base == TYPE_INT) std::cout << "int";
            else if (f->params[i].type.base == TYPE_CHAR) std::cout << "char";
            else if (f->params[i].type.base == TYPE_BOOL) std::cout << "bool";
            else if (f->params[i].type.base == TYPE_FLOAT) std::cout << "float";
            else std::cout << "void";
        }
        std::cout << ") -> ";
        if (f->returnType.isArray) std::cout << "array";
        else if (f->returnType.base == TYPE_INT) std::cout << "int";
        else if (f->returnType.base == TYPE_CHAR) std::cout << "char";
        else if (f->returnType.base == TYPE_BOOL) std::cout << "bool";
        else if (f->returnType.base == TYPE_FLOAT) std::cout << "float";
        else std::cout << "void";
        std::cout << "\n";
        printAST(f->body, indent + 1);
    } else if (VarDecl* v = dynamic_cast<VarDecl*>(node)) {
        std::cout << "Var " << v->name << ": ";
        if (v->type.isArray) std::cout << "array[" << "]";
        else if (v->type.base == TYPE_INT) std::cout << "int";
        else if (v->type.base == TYPE_CHAR) std::cout << "char";
        else if (v->type.base == TYPE_BOOL) std::cout << "bool";
        else if (v->type.base == TYPE_FLOAT) std::cout << "float";
        else std::cout << "void";
        if (v->init) {
            std::cout << " = ";
            printAST(v->init, indent);
        } else std::cout << "\n";
    } else if (BlockStmt* b = dynamic_cast<BlockStmt*>(node)) {
        std::cout << "Block\n";
        for (size_t i = 0; i < b->statements.size(); ++i)
            printAST(b->statements[i], indent + 1);
    } else if (ExprStmt* e = dynamic_cast<ExprStmt*>(node)) {
        std::cout << "ExprStmt\n";
        if (e->expr) printAST(e->expr, indent + 1);
    } else if (BinaryExpr* bin = dynamic_cast<BinaryExpr*>(node)) {
        std::cout << "Binary " << bin->op << "\n";
        printAST(bin->left, indent + 1);
        printAST(bin->right, indent + 1);
    } else if (UnaryExpr* un = dynamic_cast<UnaryExpr*>(node)) {
        std::cout << "Unary " << un->op << "\n";
        printAST(un->operand, indent + 1);
    } else if (CallExpr* call = dynamic_cast<CallExpr*>(node)) {
        std::cout << "Call " << call->funcName << "\n";
        for (size_t i = 0; i < call->args.size(); ++i)
            printAST(call->args[i], indent + 1);
    } else if (ArrayAccessExpr* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        std::cout << "ArrayAccess\n";
        printAST(arr->array, indent + 1);
        printAST(arr->index, indent + 1);
    } else if (IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(node)) {
        std::cout << "Identifier " << id->name << "\n";
    } else if (LiteralExpr* lit = dynamic_cast<LiteralExpr*>(node)) {
        std::cout << "Literal ";
        if (lit->kind == LiteralExpr::LIT_INT) std::cout << lit->value;
        else if (lit->kind == LiteralExpr::LIT_FLOAT) std::cout << lit->value;
        else if (lit->kind == LiteralExpr::LIT_CHAR) std::cout << "'" << lit->value << "'";
        else if (lit->kind == LiteralExpr::LIT_STRING) std::cout << "\"" << lit->value << "\"";
        else if (lit->kind == LiteralExpr::LIT_BOOL) std::cout << lit->value;
        std::cout << "\n";
    } else if (IfStmt* ifs = dynamic_cast<IfStmt*>(node)) {
        std::cout << "If\n";
        printAST(ifs->condition, indent + 1);
        printAST(ifs->thenBranch, indent + 1);
        if (ifs->elseBranch) {
            for (int i = 0; i < indent; ++i) std::cout << "  ";
            std::cout << "Else\n";
            printAST(ifs->elseBranch, indent + 1);
        }
    } else if (WhileStmt* wh = dynamic_cast<WhileStmt*>(node)) {
        std::cout << "While\n";
        printAST(wh->condition, indent + 1);
        printAST(wh->body, indent + 1);
    } else if (DoWhileStmt* dw = dynamic_cast<DoWhileStmt*>(node)) {
        std::cout << "DoWhile\n";
        printAST(dw->body, indent + 1);
        printAST(dw->condition, indent + 1);
    } else if (ForStmt* fr = dynamic_cast<ForStmt*>(node)) {
        std::cout << "For\n";
        if (fr->init) printAST(fr->init, indent + 1);
        else { for (int i = 0; i < indent+1; ++i) std::cout << "  "; std::cout << "(no init)\n"; }
        if (fr->condition) printAST(fr->condition, indent + 1);
        else { for (int i = 0; i < indent+1; ++i) std::cout << "  "; std::cout << "(no condition)\n"; }
        if (fr->update) printAST(fr->update, indent + 1);
        else { for (int i = 0; i < indent+1; ++i) std::cout << "  "; std::cout << "(no update)\n"; }
        printAST(fr->body, indent + 1);
    } else if (ReturnStmt* ret = dynamic_cast<ReturnStmt*>(node)) {
        std::cout << "Return";
        if (ret->expr) {
            std::cout << "\n";
            printAST(ret->expr, indent + 1);
        } else std::cout << "\n";
    } else if (BreakStmt* br = dynamic_cast<BreakStmt*>(node)) {
        std::cout << "Break\n";
    } else if (ContinueStmt* cont = dynamic_cast<ContinueStmt*>(node)) {
        std::cout << "Continue\n";
    } else if (CommaExpr* comm = dynamic_cast<CommaExpr*>(node)) {
        std::cout << "Comma\n";
        for (size_t i = 0; i < comm->exprs.size(); ++i)
            printAST(comm->exprs[i], indent + 1);
    } else {
        std::cout << "Unknown node\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << argv[0] << std::endl;
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source file>\n";
        return 1;
    }
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        for (auto tok : tokens) {
            std::cout << tok.type << " ";
            std::cout << tok.value;
            std::cout << "\n";
        }
        Parser parser(tokens);
        Program* ast = parser.parse();

        SemanticAnalyzer sem;
        sem.analyze(ast);

        printAST(ast);
        // освобождаем память
        CodeGenerator cg(ast);
        try {
            cg.generate();
        } catch (const std::exception(&e)) {
            std::cout << e.what() << std::endl;
        }
        for (auto command : cg.commands) {
            printCommand(command);
        }
        delete ast; 
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}