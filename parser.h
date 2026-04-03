#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "token.h"
#include "ast.h"

/**
 * @brief Класс синтаксического анализатора (парсера)
 * @details
 * Реализует рекурсивный спуск для разбора потока токенов и построения AST.
 * Грамматика языка построена по принципу приоритетов операторов.
 * @section usage Пример использования
 * @code
 * Lexer lexer(source);
 * std::vector<Token> tokens = lexer.tokenize();
 * Parser parser(tokens);
 * Program* ast = parser.parse();
 * 
 * @note При обнаружении синтаксической ошибки выбрасывается std::runtime_error
 * @date 2026-04-02
 */
class Parser {
public:
    /**
     * @brief Конструктор парсера
     * @param tokens Вектор токенов, полученных от лексера
     */
    Parser(const std::vector<Token>& tokens);
    
    /**
     * @brief Запуск синтаксического анализа
     * @return Корневой узел AST (Program)
     * @throw std::runtime_error При синтаксической ошибке
     * 
     * @details
     * Разбирает всю программу, начиная с правила program().
     * После успешного разбора возвращает указатель на AST.
     * Память, выделенная под AST, должна быть освобождена вызывающим кодом.
     */
    Program* parse();

private:
    std::vector<Token> tokens;  ///< Поток токенов для разбора
    size_t pos;                  ///< Текущая позиция в потоке токенов

    // === Базовые методы работы с токенами ===
    
    /**
     * @brief Просмотр текущего токена без его потребления
     * @return Константная ссылка на текущий токен
     */
    const Token& peek() const;
    
    /**
     * @brief Получение предыдущего токена
     * @return Ссылка на предыдущий токен
     * @note Предполагается, что pos > 0
     */
    Token& previous();
    
    /**
     * @brief Проверка, достигнут ли конец потока токенов
     * @return true - достигнут (TOK_EOF), false - не достигнут
     */
    bool isAtEnd() const;
    
    /**
     * @brief Переход к следующему токену
     */
    void advance();

    // === Проверка и потребление по значению (для фиксированных строк) ===
    
    /**
     * @brief Проверка, совпадает ли текущий токен с заданным значением
     * @param value Ожидаемое значение токена
     * @return true - совпадает, false - не совпадает или конец потока
     */
    bool check(const std::string& value) const;
    
    /**
     * @brief Проверка совпадения и потребление токена
     * @param value Ожидаемое значение токена
     * @return true - токен совпал и был потреблён, false - не совпал
     */
    bool match(const std::string& value);
    
    /**
     * @brief Проверка совпадения и потребление с генерацией ошибки
     * @param value Ожидаемое значение токена
     * @param msg Сообщение об ошибке
     * @return Потреблённый токен
     * @throw std::runtime_error Если токен не совпадает
     */
    Token consume(const std::string& value, const std::string& msg);

    // === Проверка и потребление по типу (для идентификаторов, чисел, литералов) ===
    
    /**
     * @brief Проверка, совпадает ли тип текущего токена с заданным
     * @param type Ожидаемый тип токена
     * @return true - тип совпадает, false - не совпадает или конец потока
     */
    bool checkType(TokenType type) const;
    
    /**
     * @brief Проверка совпадения типа и потребление токена
     * @param type Ожидаемый тип токена
     * @return true - тип совпал и токен был потреблён, false - не совпал
     */
    bool matchType(TokenType type);
    
    /**
     * @brief Проверка совпадения типа и потребление с генерацией ошибки
     * @param type Ожидаемый тип токена
     * @param msg Сообщение об ошибке
     * @return Потреблённый токен
     * @throw std::runtime_error Если тип токена не совпадает
     */
    Token consumeType(TokenType type, const std::string& msg);

    // === Удобные обёртки ===
    
    /**
     * @brief Потребление идентификатора
     * @param msg Сообщение об ошибке
     * @return Токен идентификатора
     * @throw std::runtime_error Если текущий токен не TOK_IDENTIFIER
     */
    Token consumeIdentifier(const std::string& msg);
    
    /**
     * @brief Потребление целочисленного литерала
     * @param msg Сообщение об ошибке
     * @return Токен целого числа
     * @throw std::runtime_error Если текущий токен не TOK_INTEGER
     */
    Token consumeInteger(const std::string& msg);
    
    /**
     * @brief Генерация синтаксической ошибки
     * @param tok Токен, на котором произошла ошибка
     * @param msg Сообщение об ошибке
     * @throw std::runtime_error Всегда выбрасывает исключение
     */
    void error(const Token& tok, const std::string& msg);

    // === Грамматические правила ===
    
    /**
     * @brief Разбор программы
     * @return Узел Program
     * @details program → declaration*
     */
    Program* program();
    
    /**
     * @brief Разбор объявления (верхнего уровня)
     * @return Узел объявления (VarDecl или FunctionDecl)
     * @details declaration → varDeclaration | funDeclaration
     */
    ASTNode* declaration();
    
    /**
     * @brief Разбор объявления переменной
     * @return Узел VarDecl
     * @details varDeclaration → "let" identifier ":" typeSpecifier ("=" expression)? ";"
     */
    VarDecl* varDeclaration();
    
    /**
     * @brief Разбор спецификатора типа
     * @return Структура Type
     * @details typeSpecifier → ("int" | "char" | "bool" | "void" | "float") ("[" integer "]")?
     */
    Type typeSpecifier();
    
    /**
     * @brief Разбор объявления функции
     * @return Узел FunctionDecl
     * @details funDeclaration → "fun" identifier "(" parameters? ")" ":" typeSpecifier block
     */
    FunctionDecl* funDeclaration();
    
    /**
     * @brief Разбор списка параметров функции
     * @return Вектор параметров
     * @details parameters → parameter ("," parameter)*
     * @note Возвращает пустой вектор, если параметров нет
     */
    std::vector<Parameter> parameters();
    
    /**
     * @brief Разбор блока операторов
     * @return Узел BlockStmt
     * @details block → "{" statement* "}"
     */
    BlockStmt* block();
    
    /**
     * @brief Разбор оператора
     * @return Узел оператора
     * @details statement → varDeclaration | ifStmt | forStmt | whileStmt | doWhileStmt | returnStmt | breakStmt | continueStmt | exprStmt
     */
    ASTNode* statement();
    
    /**
     * @brief Разбор условного оператора if
     * @return Узел IfStmt
     * @details ifStmt → "if" "(" expression ")" block ("else" block)?
     */
    IfStmt* ifStatement();
    
    /**
     * @brief Разбор цикла for
     * @return Узел ForStmt
     * @details forStmt → "for" "(" forInit expression? ";" expression? ")" block
     */
    ForStmt* forStatement();
    
    /**
     * @brief Разбор инициализации цикла for
     * @return Узел инициализации (VarDecl или ExprStmt)
     * @details forInit → varDeclaration | expression ";"
     */
    ASTNode* forInit();
    
    /**
     * @brief Разбор цикла while
     * @return Узел WhileStmt
     * @details whileStmt → "while" "(" expression ")" block
     */
    WhileStmt* whileStatement();
    
    /**
     * @brief Разбор цикла do-while
     * @return Узел DoWhileStmt
     * @details doWhileStmt → "do" block "while" "(" expression ")" ";"
     */
    DoWhileStmt* doWhileStatement();
    
    /**
     * @brief Разбор оператора return
     * @return Узел ReturnStmt
     * @details returnStmt → "return" expression? ";"
     */
    ReturnStmt* returnStatement();

    // === Выражения (по убыванию приоритета) ===
    
    /**
     * @brief Разбор выражения (низший приоритет)
     * @return Узел выражения
     * @details expression → assignment ("," assignment)*
     * @note Оператор запятая имеет самый низкий приоритет
     */
    Expression* expression();
    
    /**
     * @brief Разбор присваивания
     * @return Узел выражения
     * @details assignment → logicalOr (("=" | "*=" | "/=" | "%=" | "+=" | "-=") assignment)?
     * @note Присваивание правоассоциативно
     */
    Expression* assignment();
    
    /**
     * @brief Разбор логического ИЛИ
     * @return Узел выражения
     * @details logicalOr → logicalAnd ("||" logicalAnd)*
     */
    Expression* logicalOr();
    
    /**
     * @brief Разбор логического И
     * @return Узел выражения
     * @details logicalAnd → equality ("&&" equality)*
     */
    Expression* logicalAnd();
    
    /**
     * @brief Разбор сравнения на равенство
     * @return Узел выражения
     * @details equality → relational (("==" | "!=") relational)*
     */
    Expression* equality();
    
    /**
     * @brief Разбор операторов отношения
     * @return Узел выражения
     * @details relational → additive (("<" | ">" | "<=" | ">=") additive)*
     */
    Expression* relational();
    
    /**
     * @brief Разбор сложения и вычитания
     * @return Узел выражения
     * @details additive → multiplicative (("+" | "-") multiplicative)*
     */
    Expression* additive();
    
    /**
     * @brief Разбор умножения, деления и остатка
     * @return Узел выражения
     * @details multiplicative → unary (("*" | "/" | "%") unary)*
     */
    Expression* multiplicative();
    
    /**
     * @brief Разбор унарных операторов
     * @return Узел выражения
     * @details unary → ("+" | "-" | "!") unary | primary
     */
    Expression* unary();
    
    /**
     * @brief Разбор первичных выражений (высший приоритет)
     * @return Узел выражения
     * @details primary → literal | identifier ("(" arguments? ")" | "[" expression "]")? | "(" expression ")"
     */
    Expression* primary();
    
    /**
     * @brief Разбор аргументов функции
     * @return Вектор выражений-аргументов
     * @details arguments → assignment ("," assignment)*
     * @note Возвращает пустой вектор, если аргументов нет
     */
    std::vector<Expression*> arguments();
};

#endif