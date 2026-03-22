#include "lexer.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& src) : source(src), pos(0), line(1), col(1) {}

char Lexer::current() const {
    if (pos >= source.size()) return '\0';
    return source[pos];
}

void Lexer::advance() {
    if (current() == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
    pos++;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && isspace(current())) {
        advance();
    }
}

bool Lexer::isAtEnd() const {
    return pos >= source.size();
}

Token Lexer::makeToken(TokenType type, const std::string& val) {
    Token t;
    t.type = type;
    t.value = val;
    t.line = line;
    t.col = col - val.size();
    return t;
}

Token Lexer::readNumber() {
    std::string num;
    bool hasDot = false;
    while (!isAtEnd() && (isdigit(current()) || (!hasDot && current() == '.'))) {
        if (current() == '.') {
            if (hasDot) break;
            hasDot = true;
        }
        num += current();
        advance();
    }
    if (hasDot)
        return makeToken(TOK_FLOAT, num);
    else
        return makeToken(TOK_INTEGER, num);
}

Token Lexer::readIdentifierOrKeyword() {
    std::string ident;
    while (!isAtEnd() && (isalnum(current()) || current() == '_')) {
        ident += current();
        advance();
    }
    // Список ключевых слов
    static const char* keywords[] = {
        "let", "fun", "if", "else", "for", "while", "do", "return",
        "int", "char", "bool", "void", "float", "true", "false",
        "break", "continue", nullptr
    };
    for (int i = 0; keywords[i] != nullptr; ++i) {
        if (ident == keywords[i])
            return makeToken(TOK_KEYWORD, ident);
    }
    return makeToken(TOK_IDENTIFIER, ident);
}

Token Lexer::readString() {
    std::string str;
    advance(); // пропускаем "
    while (!isAtEnd() && current() != '"') {
        if (current() == '\\') {
            advance();
            if (!isAtEnd()) str += current();
        } else {
            str += current();
        }
        advance();
    }
    if (current() == '"') advance();
    else throw std::runtime_error("Unclosed string literal");
    return makeToken(TOK_STRING, str);
}

Token Lexer::readChar() {
    advance(); // пропускаем '
    char ch = current();
    advance();
    if (current() != '\'') throw std::runtime_error("Invalid char literal");
    advance(); // пропускаем '
    return makeToken(TOK_CHAR, std::string(1, ch));
}

Token Lexer::readOperatorOrDelimiter() {
    std::string op;
    op += current();
    advance();

    // Двухсимвольные операторы
    std::string two = op + current();
    if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
        two == "&&" || two == "||" || two == "*=" || two == "/=" ||
        two == "%=" || two == "+=" || two == "-=") {
        op = two;
        advance();
    }

    // Проверка на разделитель (односимвольный)
    static const std::string delimiters = ";,(){}[]:";
    if (op.size() == 1 && delimiters.find(op[0]) != std::string::npos)
        return makeToken(TOK_DELIMITER, op);
    else
        return makeToken(TOK_OPERATOR, op);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        char c = current();
        if (isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (c == '"') {
            tokens.push_back(readString());
        } else if (c == '\'') {
            tokens.push_back(readChar());
        } else {
            tokens.push_back(readOperatorOrDelimiter());
        }
    }
    tokens.push_back(makeToken(TOK_EOF, ""));
    return tokens;
}