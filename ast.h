#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "token.h"

// Базовый класс для всех узлов AST
class ASTNode {
public:
    int line, col;
    virtual ~ASTNode() {}
};

// Типы (для семантики)
enum BasicType { TYPE_INT, TYPE_CHAR, TYPE_BOOL, TYPE_FLOAT, TYPE_VOID };

struct Type {
    BasicType base;
    bool isArray;
    int arraySize; // только если isArray == true
    Type() : base(TYPE_INT), isArray(false), arraySize(0) {}
    Type(BasicType b, bool arr = false, int sz = 0) : base(b), isArray(arr), arraySize(sz) {}
};

// Литерал
class LiteralExpr : public ASTNode {
public:
    enum Kind { LIT_INT, LIT_FLOAT, LIT_CHAR, LIT_STRING, LIT_BOOL };
    Kind kind;
    std::string value; // храним как строку для простоты
    LiteralExpr(Kind k, const std::string& v, int l, int c) : kind(k), value(v) { line = l; col = c; }
};

// Идентификатор
class IdentifierExpr : public ASTNode {
public:
    std::string name;
    IdentifierExpr(const std::string& n, int l, int c) : name(n) { line = l; col = c; }
};

// Доступ к элементу массива (одномерный)
class ArrayAccessExpr : public ASTNode {
public:
    IdentifierExpr* array;
    ASTNode* index;
    ArrayAccessExpr(IdentifierExpr* arr, ASTNode* idx, int l, int c) : array(arr), index(idx) { line = l; col = c; }
    ~ArrayAccessExpr() { delete array; delete index; }
};

// Вызов функции
class CallExpr : public ASTNode {
public:
    std::string funcName;
    std::vector<ASTNode*> args;
    CallExpr(const std::string& name, const std::vector<ASTNode*>& a, int l, int c) : funcName(name), args(a) { line = l; col = c; }
    ~CallExpr() { for (size_t i = 0; i < args.size(); ++i) delete args[i]; }
};

// Унарное выражение
class UnaryExpr : public ASTNode {
public:
    std::string op;
    ASTNode* operand;
    UnaryExpr(const std::string& o, ASTNode* opnd, int l, int c) : op(o), operand(opnd) { line = l; col = c; }
    ~UnaryExpr() { delete operand; }
};

// Бинарное выражение
class BinaryExpr : public ASTNode {
public:
    std::string op;
    ASTNode* left;
    ASTNode* right;
    BinaryExpr(const std::string& o, ASTNode* l, ASTNode* r, int lc, int cc) : op(o), left(l), right(r) { line = lc; col = cc; }
    ~BinaryExpr() { delete left; delete right; }
};

// Выражение-запятая
class CommaExpr : public ASTNode {
public:
    std::vector<ASTNode*> exprs;
    CommaExpr(const std::vector<ASTNode*>& e, int l, int c) : exprs(e) { line = l; col = c; }
    ~CommaExpr() { for (size_t i = 0; i < exprs.size(); ++i) delete exprs[i]; }
};

// Объявление переменной
class VarDecl : public ASTNode {
public:
    std::string name;
    Type type;
    ASTNode* init; // может быть NULL
    VarDecl(const std::string& n, const Type& t, ASTNode* i, int l, int c) : name(n), type(t), init(i) { line = l; col = c; }
    ~VarDecl() { delete init; }
};

// Параметр функции
struct Parameter {
    std::string name;
    Type type;
    int line, col;
};

// Объявление функции
class FunctionDecl : public ASTNode {
public:
    std::string name;
    Type returnType;
    std::vector<Parameter> params;
    ASTNode* body; // блок
    FunctionDecl(const std::string& n, const Type& ret, const std::vector<Parameter>& p, ASTNode* b, int l, int c)
        : name(n), returnType(ret), params(p), body(b) { line = l; col = c; }
    ~FunctionDecl() { delete body; }
};

// Блок (составной оператор)
class BlockStmt : public ASTNode {
public:
    std::vector<ASTNode*> statements;
    BlockStmt(const std::vector<ASTNode*>& stmts, int l, int c) : statements(stmts) { line = l; col = c; }
    ~BlockStmt() { for (size_t i = 0; i < statements.size(); ++i) delete statements[i]; }
};

// Оператор-выражение
class ExprStmt : public ASTNode {
public:
    ASTNode* expr;
    ExprStmt(ASTNode* e, int l, int c) : expr(e) { line = l; col = c; }
    ~ExprStmt() { delete expr; }
};

// If-else
class IfStmt : public ASTNode {
public:
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch; // может быть NULL
    IfStmt(ASTNode* cond, ASTNode* thenb, ASTNode* elseb, int l, int c)
        : condition(cond), thenBranch(thenb), elseBranch(elseb) { line = l; col = c; }
    ~IfStmt() { delete condition; delete thenBranch; delete elseBranch; }
};

// While
class WhileStmt : public ASTNode {
public:
    ASTNode* condition;
    ASTNode* body;
    WhileStmt(ASTNode* cond, ASTNode* b, int l, int c) : condition(cond), body(b) { line = l; col = c; }
    ~WhileStmt() { delete condition; delete body; }
};

// Do-while
class DoWhileStmt : public ASTNode {
public:
    ASTNode* body;
    ASTNode* condition;
    DoWhileStmt(ASTNode* b, ASTNode* cond, int l, int c) : body(b), condition(cond) { line = l; col = c; }
    ~DoWhileStmt() { delete body; delete condition; }
};

// For
class ForStmt : public ASTNode {
public:
    ASTNode* init;   // может быть VarDecl или ExprStmt (или NULL)
    ASTNode* condition;
    ASTNode* update;
    ASTNode* body;
    ForStmt(ASTNode* i, ASTNode* cond, ASTNode* upd, ASTNode* b, int l, int c)
        : init(i), condition(cond), update(upd), body(b) { line = l; col = c; }
    ~ForStmt() { delete init; delete condition; delete update; delete body; }
};

// Return
class ReturnStmt : public ASTNode {
public:
    ASTNode* expr; // может быть NULL
    ReturnStmt(ASTNode* e, int l, int c) : expr(e) { line = l; col = c; }
    ~ReturnStmt() { delete expr; }
};

// Break
class BreakStmt : public ASTNode {
public:
    BreakStmt(int l, int c) { line = l; col = c; }
};

// Continue
class ContinueStmt : public ASTNode {
public:
    ContinueStmt(int l, int c) { line = l; col = c; }
};

// Программа
class Program : public ASTNode {
public:
    std::vector<ASTNode*> declarations;
    Program(const std::vector<ASTNode*>& decls, int l, int c) : declarations(decls) { line = l; col = c; }
    ~Program() { for (size_t i = 0; i < declarations.size(); ++i) delete declarations[i]; }
};

#endif