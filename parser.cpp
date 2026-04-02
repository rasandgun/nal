#include "parser.h"
#include <stdexcept>
#include <cctype>

Parser::Parser(const std::vector<Token>& tok) : tokens(tok), pos(0) {}

const Token& Parser::peek() const { return tokens[pos]; }
Token& Parser::previous() { return tokens[pos - 1]; }
bool Parser::isAtEnd() const { return peek().type == TOK_EOF; }
void Parser::advance() { if (!isAtEnd()) pos++; }

// === Проверка по значению ===
bool Parser::match(const std::string& value) {
    if (check(value)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(const std::string& value) const {
    if (isAtEnd()) return false;
    return peek().value == value;
}

Token Parser::consume(const std::string& value, const std::string& msg) {
    if (check(value)) {
        Token t = peek();
        advance();
        return t;
    }
    error(peek(), msg);
    Token dummy; dummy.type = TOK_EOF;
    return dummy;
}

// === Проверка по типу ===
bool Parser::matchType(TokenType type) {
    if (checkType(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::checkType(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::consumeType(TokenType type, const std::string& msg) {
    if (checkType(type)) {
        Token t = peek();
        advance();
        return t;
    }
    error(peek(), msg);
    Token dummy; dummy.type = TOK_EOF;
    return dummy;
}

// === Удобные обёртки ===
Token Parser::consumeIdentifier(const std::string& msg) {
    return consumeType(TOK_IDENTIFIER, msg);
}

Token Parser::consumeInteger(const std::string& msg) {
    return consumeType(TOK_INTEGER, msg);
}

void Parser::error(const Token& tok, const std::string& msg) {
    throw std::runtime_error("Syntax error at " + std::to_string(tok.line) + ":" +
                             std::to_string(tok.col) + " - " + msg);
}

// ==================== Грамматические правила ====================

Program* Parser::parse() { return program(); }

Program* Parser::program() {
    std::vector<ASTNode*> decls;
    int line = peek().line, col = peek().col;
    while (!isAtEnd()) {
        decls.push_back(declaration());
    }
    return new Program(decls, line, col);
}

ASTNode* Parser::declaration() {
    if (match("let")) return varDeclaration();
    if (match("fun")) return funDeclaration();
    error(peek(), "Expected declaration (let or fun)");
    return nullptr;
}

VarDecl* Parser::varDeclaration() {
    int line = previous().line, col = previous().col; // 'let'
    Token id = consumeIdentifier("Expected identifier");
    consume(":", "Expected ':'");
    Type type = typeSpecifier();
    Expression* init = nullptr;
    if (match("=")) {
        init = expression();
    }
    consume(";", "Expected ';'");
    return new VarDecl(id.value, type, init, line, col);
}

Type Parser::typeSpecifier() {
    Token base = consumeType(TOK_KEYWORD, "Expected type (int, char, bool, void, float)");
    BasicType bt;
    if (base.value == "int") bt = TYPE_INT;
    else if (base.value == "char") bt = TYPE_CHAR;
    else if (base.value == "bool") bt = TYPE_BOOL;
    else if (base.value == "void") bt = TYPE_VOID;
    else if (base.value == "float") bt = TYPE_FLOAT;
    else error(base, "Invalid type");

    bool isArray = false;
    int arraySize = 0;
    if (check("[")) {
        advance(); // съедаем '['
        isArray = true;
        // Парсим размер массива
        if (checkType(TOK_INTEGER)) {
            Token sizeToken = consumeType(TOK_INTEGER, "Expected array size");
            arraySize = std::stoi(sizeToken.value);
            if (arraySize <= 0) {
                error(sizeToken, "Array size must be positive");
            }
        } else {
            error(peek(), "Expected array size (integer)");
        }
        consume("]", "Expected ']'");
    }
    return Type(bt, isArray, arraySize);
}

FunctionDecl* Parser::funDeclaration() {
    int line = previous().line, col = previous().col; // 'fun'
    Token id = consumeIdentifier("Expected function name");
    consume("(", "Expected '('");
    std::vector<Parameter> params = parameters();
    consume(")", "Expected ')'");
    consume(":", "Expected ':'");
    Type retType = typeSpecifier();
    BlockStmt* body = block();
    return new FunctionDecl(id.value, retType, params, body, line, col);
}

std::vector<Parameter> Parser::parameters() {
    std::vector<Parameter> params;
    if (check(")")) return params; // нет параметров
    while (true) {
        Token id = consumeIdentifier("Expected parameter name");
        consume(":", "Expected ':'");
        Type t = typeSpecifier();
        Parameter p;
        p.name = id.value;
        p.type = t;
        p.line = id.line;
        p.col = id.col;
        params.push_back(p);
        if (!match(",")) break;
    }
    return params;
}

BlockStmt* Parser::block() {
    int line = peek().line, col = peek().col;
    consume("{", "Expected '{'");
    std::vector<ASTNode*> stmts;
    while (!check("}")) {
        stmts.push_back(statement());
    }
    consume("}", "Expected '}'");
    return new BlockStmt(stmts, line, col);
}

ASTNode* Parser::statement() {
    if (check("let")) { advance(); return varDeclaration(); }
    if (check("if"))  { advance(); return ifStatement(); }
    if (check("for")) { advance(); return forStatement(); }
    if (check("while")) { advance(); return whileStatement(); }
    if (check("do")) { advance(); return doWhileStatement(); }
    if (check("return")) { advance(); return returnStatement(); }
    if (check("break")) {
        advance();
        int l = previous().line, c = previous().col;
        consume(";", "Expected ';'");
        return new BreakStmt(l, c);
    }
    if (check("continue")) {
        advance();
        int l = previous().line, c = previous().col;
        consume(";", "Expected ';'");
        return new ContinueStmt(l, c);
    }
    // иначе это выражение-оператор
    Expression* expr = expression();
    consume(";", "Expected ';'");
    return new ExprStmt(expr, expr->line, expr->col);
}

IfStmt* Parser::ifStatement() {
    int line = previous().line, col = previous().col;
    consume("(", "Expected '('");
    Expression* cond = expression();
    consume(")", "Expected ')'");
    ASTNode* thenBranch = block();
    ASTNode* elseBranch = nullptr;
    if (match("else")) {
        elseBranch = block();
    }
    return new IfStmt(cond, thenBranch, elseBranch, line, col);
}

ForStmt* Parser::forStatement() {
    int line = previous().line, col = previous().col;
    consume("(", "Expected '('");
    ASTNode* init = forInit();
    Expression* cond = expression();
    consume(";", "Expected ';'");
    Expression* update = nullptr;
    if (!check(")")) {
        update = expression();
    }
    consume(")", "Expected ')'");
    ASTNode* body = block();
    return new ForStmt(init, cond, update, body, line, col);
}

ASTNode* Parser::forInit() {
    if (check("let")) {
        advance();
        return varDeclaration();
    } else {
        Expression* expr = expression();
        consume(";", "Expected ';'");
        return new ExprStmt(expr, expr->line, expr->col);
    }
}

WhileStmt* Parser::whileStatement() {
    int line = previous().line, col = previous().col;
    consume("(", "Expected '('");
    Expression* cond = expression();
    consume(")", "Expected ')'");
    ASTNode* body = block();
    return new WhileStmt(cond, body, line, col);
}

DoWhileStmt* Parser::doWhileStatement() {
    int line = previous().line, col = previous().col;
    ASTNode* body = block();
    consume("while", "Expected 'while'");
    consume("(", "Expected '('");
    Expression* cond = expression();
    consume(")", "Expected ')'");
    consume(";", "Expected ';'");
    return new DoWhileStmt(body, cond, line, col);
}

ReturnStmt* Parser::returnStatement() {
    int line = previous().line, col = previous().col;
    Expression* expr = nullptr;
    if (!check(";")) {
        expr = expression();
    }
    consume(";", "Expected ';'");
    return new ReturnStmt(expr, line, col);
}

// === Выражения ===
Expression* Parser::expression() {
    Expression* left = assignment();
    if (match(",")) {
        std::vector<Expression*> exprs;
        exprs.push_back(left);
        do {
            exprs.push_back(assignment());
        }while (match(","));
        return new CommaExpr(exprs, left->line, left->col);
    }
    return left;
}

Expression* Parser::assignment() {
    Expression* left = logicalOr();
    // операторы присваивания: =, *=, /=, %=, +=, -=
    if (match("=") || match("*=") || match("/=") || match("%=") || match("+=") || match("-=")) {
        std::string op = previous().value;
        Expression* right = assignment();
        return new BinaryExpr(op, left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::logicalOr() {
    Expression* left = logicalAnd();
    while (match("||")) {
        Expression* right = logicalAnd();
        left = new BinaryExpr("||", left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::logicalAnd() {
    Expression* left = equality();
    while (match("&&")) {
        Expression* right = equality();
        left = new BinaryExpr("&&", left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::equality() {
    Expression* left = relational();
    while (match("==") || match("!=")) {
        std::string op = previous().value;
        Expression* right = relational();
        left = new BinaryExpr(op, left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::relational() {
    Expression* left = additive();
    while (match("<") || match(">") || match("<=") || match(">=")) {
        std::string op = previous().value;
        Expression* right = additive();
        left = new BinaryExpr(op, left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::additive() {
    Expression* left = multiplicative();
    while (match("+") || match("-")) {
        std::string op = previous().value;
        Expression* right = multiplicative();
        left = new BinaryExpr(op, left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::multiplicative() {
    Expression* left = unary();
    while (match("*") || match("/") || match("%")) {
        std::string op = previous().value;
        Expression* right = unary();
        left = new BinaryExpr(op, left, right, left->line, left->col);
    }
    return left;
}

Expression* Parser::unary() {
    if (match("+") || match("-") || match("!")) {
        std::string op = previous().value;
        Expression* operand = unary();
        return new UnaryExpr(op, operand, previous().line, previous().col);
    }
    return primary();
}

Expression* Parser::primary() {
    if (matchType(TOK_INTEGER)) {
        return new LiteralExpr(LiteralExpr::LIT_INT, previous().value, previous().line, previous().col);
    } else if (matchType(TOK_FLOAT)) {
        return new LiteralExpr(LiteralExpr::LIT_FLOAT, previous().value, previous().line, previous().col);
    } else if (matchType(TOK_CHAR)) {
        return new LiteralExpr(LiteralExpr::LIT_CHAR, previous().value, previous().line, previous().col);
    } else if (matchType(TOK_STRING)) {
        return new LiteralExpr(LiteralExpr::LIT_STRING, previous().value, previous().line, previous().col);
    } else if (match("true") || match("false")) {
        return new LiteralExpr(LiteralExpr::LIT_BOOL, previous().value, previous().line, previous().col);
    } else if (matchType(TOK_IDENTIFIER)) {
        std::string name = previous().value;
        int l = previous().line, c = previous().col;
        if (match("(")) {
            std::vector<Expression*> args = arguments();
            consume(")", "Expected ')'");
            return new CallExpr(name, args, l, c);
        } else if (check("[")) {
            advance(); // '['
            Expression* idx = expression();
            consume("]", "Expected ']'");
            return new ArrayAccessExpr(new IdentifierExpr(name, l, c), idx, l, c);
        } else {
            return new IdentifierExpr(name, l, c);
        }
    } else if (match("(")) {
        Expression* expr = expression();
        consume(")", "Expected ')'");
        return expr;
    }
    error(peek(), "Expected expression");
    return nullptr;
}

std::vector<Expression*> Parser::arguments() {
    std::vector<Expression*> args;
    if (check(")")) return args;
    do {
        args.push_back(assignment());
    } while (match(","));
    return args;
}