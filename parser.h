#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "token.h"
#include "ast.h"

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    Program* parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& peek() const;
    Token& previous();
    bool isAtEnd() const;
    void advance();

    // Проверка и потребление по значению (для фиксированных строк)
    bool match(const std::string& value);
    bool check(const std::string& value) const;
    Token consume(const std::string& value, const std::string& msg);

    // Проверка и потребление по типу (для идентификаторов, чисел, литералов)
    bool matchType(TokenType type);
    bool checkType(TokenType type) const;
    Token consumeType(TokenType type, const std::string& msg);

    // Удобные обёртки
    Token consumeIdentifier(const std::string& msg);
    Token consumeInteger(const std::string& msg);
    // можно добавить для других типов при необходимости

    void error(const Token& tok, const std::string& msg);

    // Грамматические правила
    Program* program();
    ASTNode* declaration();
    VarDecl* varDeclaration();
    Type typeSpecifier();
    FunctionDecl* funDeclaration();
    std::vector<Parameter> parameters();
    BlockStmt* block();
    ASTNode* statement();
    IfStmt* ifStatement();
    ForStmt* forStatement();
    ASTNode* forInit();
    WhileStmt* whileStatement();
    DoWhileStmt* doWhileStatement();
    ReturnStmt* returnStatement();

    ASTNode* expression();
    ASTNode* assignment();
    ASTNode* logicalOr();
    ASTNode* logicalAnd();
    ASTNode* equality();
    ASTNode* relational();
    ASTNode* additive();
    ASTNode* multiplicative();
    ASTNode* unary();
    ASTNode* primary();
    std::vector<ASTNode*> arguments();
};

#endif