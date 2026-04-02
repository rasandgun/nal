#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "token.h"

// Типы (для семантики)
enum BasicType { TYPE_INT, TYPE_CHAR, TYPE_BOOL, TYPE_FLOAT, TYPE_VOID };

struct Type {
    BasicType base;
    bool isArray;
    int size;          // размер для массива (0 для не-массива)
    Type() : base(TYPE_INT), isArray(false), size(0) {}
    Type(BasicType b, bool arr = false, int sz = 0) 
        : base(b), isArray(arr), size(sz) {}
};

// Базовый класс для всех узлов AST
class ASTNode {
public:
    int line, col;
    ASTNode(int l, int c) : line(l), col(c) {}
    virtual ~ASTNode() {}
};

class Expression : public ASTNode {
public:
    Type type;
    Expression(int l, int c) : ASTNode(l, c) {}
    virtual ~Expression() {}
};

// Литерал
class LiteralExpr : public Expression {
public:
    enum Kind { LIT_INT, LIT_FLOAT, LIT_CHAR, LIT_STRING, LIT_BOOL };
    Kind kind;
    std::string value;
    LiteralExpr(Kind k, const std::string& v, int l, int c) 
        : Expression(l, c), kind(k), value(v) {}
};

// Идентификатор
class IdentifierExpr : public Expression {
public:
    std::string name;
    IdentifierExpr(const std::string& n, int l, int c) 
        : Expression(l, c), name(n) {}
};

// Доступ к элементу массива (одномерный)
class ArrayAccessExpr : public Expression {
public:
    IdentifierExpr* array;
    Expression* index;
    ArrayAccessExpr(IdentifierExpr* arr, Expression* idx, int l, int c) 
        : Expression(l, c), array(arr), index(idx) {}
    ~ArrayAccessExpr() { delete array; delete index; }
};

// Вызов функции
class CallExpr : public Expression {
public:
    std::string funcName;
    std::vector<Expression*> args;
    CallExpr(const std::string& name, const std::vector<Expression*>& a, int l, int c) 
        : Expression(l, c), funcName(name), args(a) {}
    ~CallExpr() { for (size_t i = 0; i < args.size(); ++i) delete args[i]; }
};

// Унарное выражение
class UnaryExpr : public Expression {
public:
    std::string op;
    Expression* operand;
    UnaryExpr(const std::string& o, Expression* opnd, int l, int c) 
        : Expression(l, c), op(o), operand(opnd) {}
    ~UnaryExpr() { delete operand; }
};

// Бинарное выражение
class BinaryExpr : public Expression {
public:
    std::string op;
    Expression* left;
    Expression* right;
    BinaryExpr(const std::string& o, Expression* l, Expression* r, int lc, int cc) 
        : Expression(lc, cc), op(o), left(l), right(r) {}
    ~BinaryExpr() { delete left; delete right; }
};

// Выражение-запятая
class CommaExpr : public Expression {
public:
    std::vector<Expression*> exprs;
    CommaExpr(const std::vector<Expression*>& e, int l, int c) 
        : Expression(l, c), exprs(e) {}
    ~CommaExpr() { for (size_t i = 0; i < exprs.size(); ++i) delete exprs[i]; }
};

// Объявление переменной
class VarDecl : public ASTNode {
public:
    std::string name;
    Type type;
    Expression* init;
    VarDecl(const std::string& n, const Type& t, Expression* i, int l, int c) 
        : ASTNode(l, c), name(n), type(t), init(i) {}
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
    ASTNode* body;
    FunctionDecl(const std::string& n, const Type& ret, const std::vector<Parameter>& p, ASTNode* b, int l, int c)
        : ASTNode(l, c), name(n), returnType(ret), params(p), body(b) {}
    ~FunctionDecl() { delete body; }
};

// Блок (составной оператор)
class BlockStmt : public ASTNode {
public:
    std::vector<ASTNode*> statements;
    BlockStmt(const std::vector<ASTNode*>& stmts, int l, int c) 
        : ASTNode(l, c), statements(stmts) {}
    ~BlockStmt() { for (size_t i = 0; i < statements.size(); ++i) delete statements[i]; }
};

// Оператор-выражение
class ExprStmt : public ASTNode {
public:
    Expression* expr;
    ExprStmt(Expression* e, int l, int c) 
        : ASTNode(l, c), expr(e) {}
    ~ExprStmt() { delete expr; }
};

// If-else
class IfStmt : public ASTNode {
public:
    Expression* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;
    IfStmt(Expression* cond, ASTNode* thenb, ASTNode* elseb, int l, int c)
        : ASTNode(l, c), condition(cond), thenBranch(thenb), elseBranch(elseb) {}
    ~IfStmt() { delete condition; delete thenBranch; delete elseBranch; }
};

// While
class WhileStmt : public ASTNode {
public:
    Expression* condition;
    ASTNode* body;
    WhileStmt(Expression* cond, ASTNode* b, int l, int c) 
        : ASTNode(l, c), condition(cond), body(b) {}
    ~WhileStmt() { delete condition; delete body; }
};

// Do-while
class DoWhileStmt : public ASTNode {
public:
    ASTNode* body;
    Expression* condition;
    DoWhileStmt(ASTNode* b, Expression* cond, int l, int c) 
        : ASTNode(l, c), body(b), condition(cond) {}
    ~DoWhileStmt() { delete body; delete condition; }
};

// For
class ForStmt : public ASTNode {
public:
    ASTNode* init;
    Expression* condition;
    Expression* update;
    ASTNode* body;
    ForStmt(ASTNode* i, Expression* cond, Expression* upd, ASTNode* b, int l, int c)
        : ASTNode(l, c), init(i), condition(cond), update(upd), body(b) {}
    ~ForStmt() { delete init; delete condition; delete update; delete body; }
};

// Return
class ReturnStmt : public ASTNode {
public:
    Expression* expr;
    ReturnStmt(Expression* e, int l, int c) 
        : ASTNode(l, c), expr(e) {}
    ~ReturnStmt() { delete expr; }
};

// Break
class BreakStmt : public ASTNode {
public:
    BreakStmt(int l, int c) : ASTNode(l, c) {}
};

// Continue
class ContinueStmt : public ASTNode {
public:
    ContinueStmt(int l, int c) : ASTNode(l, c) {}
};

// Программа
class Program : public ASTNode {
public:
    std::vector<ASTNode*> declarations;
    Program(const std::vector<ASTNode*>& decls, int l, int c) 
        : ASTNode(l, c), declarations(decls) {}
    ~Program() { for (size_t i = 0; i < declarations.size(); ++i) delete declarations[i]; }
};

#endif