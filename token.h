#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
  TOK_KEYWORD,
  TOK_IDENTIFIER,
  TOK_INTEGER,
  TOK_FLOAT,
  TOK_CHAR,
  TOK_STRING,
  TOK_OPERATOR,
  TOK_DELIMITER,
  TOK_EOF
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;
};

#endif