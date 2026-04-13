#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include "token.h"

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos;
    int line, col;

    char current() const;
    void advance();
    void skipWhitespace();
    bool isAtEnd() const;
    Token makeToken(TokenType type, const std::string& val);
    Token readNumber();
    Token readIdentifierOrKeyword();
    Token readString();
    Token readChar();
    Token readOperatorOrDelimiter();
    char peek() const;
    void skipComment();
};

#endif