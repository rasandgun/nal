#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "token.h"

/**
 * @brief Базовые типы данных языка
 * 
 * Определяет основные типы, поддерживаемые компилятором:
 * - TYPE_INT: целочисленный тип
 * - TYPE_CHAR: символьный тип
 * - TYPE_BOOL: логический тип (true/false)
 * - TYPE_FLOAT: тип с плавающей точкой
 * - TYPE_VOID: тип для функций, не возвращающих значение
 */
enum BasicType { TYPE_INT, TYPE_CHAR, TYPE_BOOL, TYPE_FLOAT, TYPE_VOID };

/**
 * @brief Структура для полного описания типа переменной
 * 
 * Позволяет описывать как простые типы, так и массивы.
 * 
 * @code
 * Type intType(TYPE_INT);           // просто int
 * Type intArray(TYPE_INT, true, 10); // массив из 10 int
 * @endcode
 * 
 * @date 2026-04-02
 */
struct Type {
    BasicType base;     ///< Базовый тип (int, char, bool, float, void)
    bool isArray;       ///< Флаг, является ли тип массивом
    int size;           ///< Размер массива (0 для не-массива)
    
    /**
     * @brief Конструктор по умолчанию
     * @details Создаёт тип int, не массив
     */
    Type() : base(TYPE_INT), isArray(false), size(0) {}
    
    /**
     * @brief Конструктор с параметрами
     * @param b Базовый тип
     * @param arr Флаг массива (по умолчанию false)
     * @param sz Размер массива (по умолчанию 0)
     */
    Type(BasicType b, bool arr = false, int sz = 0) 
        : base(b), isArray(arr), size(sz) {}
};

/**
 * @brief Базовый класс для всех узлов абстрактного синтаксического дерева (AST)
 * 
 * AST (Abstract Syntax Tree) представляет программу в древовидной структуре,
 * где каждый узел соответствует языковой конструкции.
 * 
 * @note Все узлы наследуют позицию в исходном коде (строку и колонку)
 * @date 2026-04-02
 */
class ASTNode {
public:
    int line;   ///< Номер строки в исходном коде (начиная с 1)
    int col;    ///< Номер колонки в строке (начиная с 1)
    
    /**
     * @brief Конструктор узла AST
     * @param l Номер строки
     * @param c Номер колонки
     */
    ASTNode(int l, int c) : line(l), col(c) {}
    
    /**
     * @brief Виртуальный деструктор для корректного удаления производных классов
     */
    virtual ~ASTNode() {}
};

/**
 * @brief Базовый класс для всех выражений в AST
 * 
 * Выражение — это конструкция языка, которая вычисляется и возвращает значение.
 * Примеры: литералы, арифметические операции, вызовы функций.
 * 
 * @see LiteralExpr, BinaryExpr, CallExpr
 * @date 2026-04-02
 */
class Expression : public ASTNode {
public:
    Type type;  ///< Тип результата выражения (вычисляется семантическим анализатором)
    
    /**
     * @brief Конструктор выражения
     * @param l Номер строки
     * @param c Номер колонки
     */
    Expression(int l, int c) : ASTNode(l, c) {}
    
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~Expression() {}
};

/**
 * @brief Узел AST, соответствующий литералу (константному значению)
 * 
 * Литералы — это значения, записанные непосредственно в коде:
 * - Целые числа: 42, -17
 * - Числа с плавающей точкой: 3.14, -0.5
 * - Символы: 'a', '\n'
 * - Строки: "Hello, world!"
 * - Булевы значения: true, false
 * 
 * @code
 * LiteralExpr* intLit = new LiteralExpr(LiteralExpr::LIT_INT, "42", line, col);
 * LiteralExpr* strLit = new LiteralExpr(LiteralExpr::LIT_STRING, "Hello", line, col);
 * @endcode
 */
class LiteralExpr : public Expression {
public:
    /**
     * @brief Типы литералов
     */
    enum Kind { 
        LIT_INT,     ///< Целочисленный литерал (например, 123)
        LIT_FLOAT,   ///< Литерал с плавающей точкой (например, 3.14)
        LIT_CHAR,    ///< Символьный литерал (например, 'a')
        LIT_STRING,  ///< Строковый литерал (например, "hello")
        LIT_BOOL     ///< Булев литерал (true или false)
    };
    
    Kind kind;          ///< Тип литерала
    std::string value;  ///< Строковое представление значения
    
    /**
     * @brief Конструктор литерала
     * @param k Тип литерала
     * @param v Строковое представление значения
     * @param l Номер строки
     * @param c Номер колонки
     */
    LiteralExpr(Kind k, const std::string& v, int l, int c) 
        : Expression(l, c), kind(k), value(v) {}
};

/**
 * @brief Узел AST, соответствующий обращению к переменной
 * 
 * Представляет использование переменной в выражении.
 * Пример: x, count, array[i]
 * 
 * @code
 * IdentifierExpr* var = new IdentifierExpr("x", line, col);
 * @endcode
 * 
 * @note Семантический анализатор проверяет, что переменная объявлена
 */
class IdentifierExpr : public Expression {
public:
    std::string name;   ///< Имя переменной или функции
    
    /**
     * @brief Конструктор идентификатора
     * @param n Имя идентификатора
     * @param l Номер строки
     * @param c Номер колонки
     */
    IdentifierExpr(const std::string& n, int l, int c) 
        : Expression(l, c), name(n) {}
};

/**
 * @brief Узел AST, соответствующий обращению к элементу массива
 * 
 * Представляет доступ к элементу массива по индексу.
 * Пример: arr[5], matrix[i][j] (через вложенные ArrayAccessExpr)
 * 
 * @code
 * IdentifierExpr* array = new IdentifierExpr("arr", line, col);
 * Expression* index = new LiteralExpr(LIT_INT, "5", line, col);
 * ArrayAccessExpr* access = new ArrayAccessExpr(array, index, line, col);
 * @endcode
 * 
 * @note Индекс должен быть целочисленного типа
 */
class ArrayAccessExpr : public Expression {
public:
    IdentifierExpr* array;  ///< Имя массива (передаётся владение)
    Expression* index;      ///< Выражение-индекс (передаётся владение)
    
    /**
     * @brief Конструктор доступа к массиву
     * @param arr Идентификатор массива
     * @param idx Выражение-индекс
     * @param l Номер строки
     * @param c Номер колонки
     */
    ArrayAccessExpr(IdentifierExpr* arr, Expression* idx, int l, int c) 
        : Expression(l, c), array(arr), index(idx) {}
    
    /**
     * @brief Деструктор, освобождающий память array и index
     */
    ~ArrayAccessExpr() { delete array; delete index; }
};

/**
 * @brief Узел AST, соответствующий вызову функции
 * 
 * Представляет вызов функции с аргументами.
 * Пример: foo(), bar(1, 2), printf("Hello")
 * 
 * @code
 * std::vector<Expression*> args;
 * args.push_back(new LiteralExpr(LIT_INT, "42", line, col));
 * CallExpr* call = new CallExpr("print", args, line, col);
 * @endcode
 * 
 * @note Семантический анализатор проверяет соответствие типов аргументов
 */
class CallExpr : public Expression {
public:
    std::string funcName;                       ///< Имя вызываемой функции
    std::vector<Expression*> args;              ///< Список аргументов (передаётся владение)
    
    /**
     * @brief Конструктор вызова функции
     * @param name Имя функции
     * @param a Вектор аргументов
     * @param l Номер строки
     * @param c Номер колонки
     */
    CallExpr(const std::string& name, const std::vector<Expression*>& a, int l, int c) 
        : Expression(l, c), funcName(name), args(a) {}
    
    /**
     * @brief Деструктор, освобождающий память всех аргументов
     */
    ~CallExpr() { for (size_t i = 0; i < args.size(); ++i) delete args[i]; }
};

/**
 * @brief Узел AST, соответствующий унарному выражению
 * 
 * Унарные операторы применяются к одному операнду.
 * Поддерживаемые операторы:
 * - + : унарный плюс (не меняет значение)
 * - - : унарный минус (смена знака)
 * - ! : логическое отрицание
 * 
 * @code
 * UnaryExpr* neg = new UnaryExpr("-", operand, line, col);
 * UnaryExpr* not = new UnaryExpr("!", operand, line, col);
 * @endcode
 * 
 * @see BinaryExpr
 */
class UnaryExpr : public Expression {
public:
    std::string op;         ///< Оператор (+, -, !)
    Expression* operand;    ///< Операнд (передаётся владение)
    
    /**
     * @brief Конструктор унарного выражения
     * @param o Строка оператора
     * @param opnd Операнд
     * @param l Номер строки
     * @param c Номер колонки
     */
    UnaryExpr(const std::string& o, Expression* opnd, int l, int c) 
        : Expression(l, c), op(o), operand(opnd) {}
    
    /**
     * @brief Деструктор, освобождающий память операнда
     */
    ~UnaryExpr() { delete operand; }
};

/**
 * @brief Узел AST, соответствующий бинарному выражению
 * 
 * Бинарные операторы применяются к двум операндам.
 * Поддерживаемые группы операторов:
 * - Арифметические: +, -, *, /, %
 * - Сравнения: ==, !=, <, >, <=, >=
 * - Логические: &&, ||
 * - Присваивания: =, +=, -=, *=, /=, %=
 * 
 * @code
 * BinaryExpr* sum = new BinaryExpr("+", left, right, line, col);
 * BinaryExpr* assign = new BinaryExpr("=", var, value, line, col);
 * @endcode
 * 
 * @warning Для операторов присваивания левый операнд должен быть lvalue
 * 
 * @see UnaryExpr
 */
class BinaryExpr : public Expression {
public:
    std::string op;         ///< Оператор (+, -, *, /, =, ==, && и т.д.)
    Expression* left;       ///< Левый операнд (передаётся владение)
    Expression* right;      ///< Правый операнд (передаётся владение)
    
    /**
     * @brief Конструктор бинарного выражения
     * @param o Строка оператора
     * @param l Левый операнд
     * @param r Правый операнд
     * @param lc Номер строки
     * @param cc Номер колонки
     */
    BinaryExpr(const std::string& o, Expression* l, Expression* r, int lc, int cc) 
        : Expression(lc, cc), op(o), left(l), right(r) {}
    
    /**
     * @brief Деструктор, освобождающий память операндов
     */
    ~BinaryExpr() { delete left; delete right; }
};

/**
 * @brief Узел AST, соответствующий выражению с оператором запятая
 * 
 * Оператор запятая (,) последовательно вычисляет несколько выражений
 * и возвращает значение последнего.
 * 
 * Пример: a = 1, b = 2, a + b  // возвращает 3
 * 
 */
class CommaExpr : public Expression {
public:
    std::vector<Expression*> exprs;  ///< Последовательность выражений (передаётся владение)
    
    /**
     * @brief Конструктор выражения-запятая
     * @param e Вектор выражений
     * @param l Номер строки
     * @param c Номер колонки
     */
    CommaExpr(const std::vector<Expression*>& e, int l, int c) 
        : Expression(l, c), exprs(e) {}
    
    /**
     * @brief Деструктор, освобождающий память всех выражений
     */
    ~CommaExpr() { for (size_t i = 0; i < exprs.size(); ++i) delete exprs[i]; }
};

/**
 * @brief Узел AST, соответствующий объявлению переменной
 * 
 * Объявление переменной с указанием типа и опциональной инициализацией.
 * 
 * @code
 * // let x: int = 10;
 * Type intType(TYPE_INT);
 * VarDecl* var = new VarDecl("x", intType, new LiteralExpr(LIT_INT, "10", l, c), l, c);
 * 
 * // let arr: int[5];
 * Type arrType(TYPE_INT, true, 5);
 * VarDecl* array = new VarDecl("arr", arrType, nullptr, l, c);
 * @endcode
 * 
 * @note Семантический анализатор проверяет, что тип инициализатора совместим с типом переменной
 */
class VarDecl : public ASTNode {
public:
    std::string name;       ///< Имя переменной
    Type type;              ///< Тип переменной
    Expression* init;       ///< Выражение-инициализатор (может быть nullptr)
    
    /**
     * @brief Конструктор объявления переменной
     * @param n Имя переменной
     * @param t Тип переменной
     * @param i Инициализатор (nullptr если без инициализации)
     * @param l Номер строки
     * @param c Номер колонки
     */
    VarDecl(const std::string& n, const Type& t, Expression* i, int l, int c) 
        : ASTNode(l, c), name(n), type(t), init(i) {}
    
    /**
     * @brief Деструктор, освобождающий память инициализатора
     */
    ~VarDecl() { delete init; }
};

/**
 * @brief Структура, описывающая параметр функции
 * 
 * Содержит имя параметра, его тип и позицию в исходном коде.
 * 
 * @code
 * Parameter p;
 * p.name = "x";
 * p.type = Type(TYPE_INT);
 * p.line = 10;
 * p.col = 15;
 * @endcode
 */
struct Parameter {
    std::string name;   ///< Имя параметра
    Type type;          ///< Тип параметра
    int line;           ///< Номер строки в исходном коде
    int col;            ///< Номер колонки в строке
};

/**
 * @brief Узел AST, соответствующий объявлению функции
 * 
 * Объявление функции включает имя, возвращаемый тип, список параметров и тело.
 * 
 * @code
 * std::vector<Parameter> params;
 * Parameter p = {"a", Type(TYPE_INT), line, col};
 * params.push_back(p);
 * 
 * Type retType(TYPE_INT);
 * BlockStmt* body = new BlockStmt(statements, line, col);
 * FunctionDecl* func = new FunctionDecl("add", retType, params, body, line, col);
 * @endcode
 * 
 * @note Тело функции — это блок операторов (BlockStmt)
 */
class FunctionDecl : public ASTNode {
public:
    std::string name;                       ///< Имя функции
    Type returnType;                        ///< Возвращаемый тип
    std::vector<Parameter> params;          ///< Список параметров
    ASTNode* body;                          ///< Тело функции (обычно BlockStmt)
    
    /**
     * @brief Конструктор объявления функции
     * @param n Имя функции
     * @param ret Возвращаемый тип
     * @param p Список параметров
     * @param b Тело функции
     * @param l Номер строки
     * @param c Номер колонки
     */
    FunctionDecl(const std::string& n, const Type& ret, const std::vector<Parameter>& p, ASTNode* b, int l, int c)
        : ASTNode(l, c), name(n), returnType(ret), params(p), body(b) {}
    
    /**
     * @brief Деструктор, освобождающий память тела функции
     */
    ~FunctionDecl() { delete body; }
};

/**
 * @brief Узел AST, соответствующий блоку (составному оператору)
 * 
 * Блок — это последовательность операторов, заключённая в фигурные скобки {}.
 * Блок создаёт новую область видимости.
 * 
 * @code
 * std::vector<ASTNode*> stmts;
 * stmts.push_back(varDecl);
 * stmts.push_back(exprStmt);
 * BlockStmt* block = new BlockStmt(stmts, line, col);
 * @endcode
 * 
 * @note Переменные, объявленные внутри блока, не видны за его пределами
 */
class BlockStmt : public ASTNode {
public:
    std::vector<ASTNode*> statements;  ///< Список операторов в блоке
    
    /**
     * @brief Конструктор блока
     * @param stmts Вектор операторов
     * @param l Номер строки
     * @param c Номер колонки
     */
    BlockStmt(const std::vector<ASTNode*>& stmts, int l, int c) 
        : ASTNode(l, c), statements(stmts) {}
    
    /**
     * @brief Деструктор, освобождающий память всех операторов
     */
    ~BlockStmt() { for (size_t i = 0; i < statements.size(); ++i) delete statements[i]; }
};

/**
 * @brief Узел AST, соответствующий оператору-выражению
 * 
 * Выражение, за которым следует точка с запятой, становится оператором.
 * Примеры:
 * - x = 5;
 * - foo();
 * - a + b;  // бессмысленно, но синтаксически верно
 * 
 * @code
 * Expression* expr = new BinaryExpr("=", var, value, line, col);
 * ExprStmt* stmt = new ExprStmt(expr, line, col);
 * @endcode
 */
class ExprStmt : public ASTNode {
public:
    Expression* expr;  ///< Выражение-оператор (передаётся владение)
    
    /**
     * @brief Конструктор оператора-выражения
     * @param e Выражение
     * @param l Номер строки
     * @param c Номер колонки
     */
    ExprStmt(Expression* e, int l, int c) 
        : ASTNode(l, c), expr(e) {}
    
    /**
     * @brief Деструктор, освобождающий память выражения
     */
    ~ExprStmt() { delete expr; }
};

/**
 * @brief Узел AST, соответствующий условному оператору if-else
 * 
 * Условное выполнение блока кода в зависимости от условия.
 * 
 * @code
 * IfStmt* ifstmt = new IfStmt(condition, thenBlock, elseBlock, line, col);
 * @endcode
 * 
 * @note Условие должно быть скалярного типа (int, char, bool)
 * @note elseBranch может быть nullptr, если ветка else отсутствует
 */
class IfStmt : public ASTNode {
public:
    Expression* condition;   ///< Условие (передаётся владение)
    ASTNode* thenBranch;     ///< Ветка then (выполняется если условие истинно)
    ASTNode* elseBranch;     ///< Ветка else (выполняется если условие ложно, может быть nullptr)
    
    /**
     * @brief Конструктор условного оператора
     * @param cond Условие
     * @param thenb Ветка then
     * @param elseb Ветка else (nullptr если нет)
     * @param l Номер строки
     * @param c Номер колонки
     */
    IfStmt(Expression* cond, ASTNode* thenb, ASTNode* elseb, int l, int c)
        : ASTNode(l, c), condition(cond), thenBranch(thenb), elseBranch(elseb) {}
    
    /**
     * @brief Деструктор, освобождающий память всех компонентов
     */
    ~IfStmt() { delete condition; delete thenBranch; delete elseBranch; }
};

/**
 * @brief Узел AST, соответствующий циклу while
 * 
 * Цикл с предусловием: тело выполняется, пока условие истинно.
 * 
 * @code
 * WhileStmt* whileLoop = new WhileStmt(condition, body, line, col);
 * @endcode
 * 
 * @note Условие проверяется перед каждым выполнением тела
 * @note Тело может не выполниться ни разу, если условие изначально ложно
 */
class WhileStmt : public ASTNode {
public:
    Expression* condition;   ///< Условие продолжения цикла
    ASTNode* body;           ///< Тело цикла
    
    /**
     * @brief Конструктор цикла while
     * @param cond Условие
     * @param b Тело цикла
     * @param l Номер строки
     * @param c Номер колонки
     */
    WhileStmt(Expression* cond, ASTNode* b, int l, int c) 
        : ASTNode(l, c), condition(cond), body(b) {}
    
    /**
     * @brief Деструктор, освобождающий память
     */
    ~WhileStmt() { delete condition; delete body; }
};

/**
 * @brief Узел AST, соответствующий циклу do-while
 * 
 * Цикл с постусловием: тело выполняется хотя бы один раз,
 * затем повторяется, пока условие истинно.
 * 
 * @code
 * DoWhileStmt* doWhile = new DoWhileStmt(body, condition, line, col);
 * @endcode
 * 
 * @note Условие проверяется после выполнения тела
 * @note Тело всегда выполняется как минимум один раз
 */
class DoWhileStmt : public ASTNode {
public:
    ASTNode* body;           ///< Тело цикла
    Expression* condition;   ///< Условие продолжения цикла
    
    /**
     * @brief Конструктор цикла do-while
     * @param b Тело цикла
     * @param cond Условие
     * @param l Номер строки
     * @param c Номер колонки
     */
    DoWhileStmt(ASTNode* b, Expression* cond, int l, int c) 
        : ASTNode(l, c), body(b), condition(cond) {}
    
    /**
     * @brief Деструктор, освобождающий память
     */
    ~DoWhileStmt() { delete body; delete condition; }
};

/**
 * @brief Узел AST, соответствующий циклу for
 * 
 * Цикл for с инициализацией, условием и обновлением.
 * 
 * @code
 * ForStmt* forLoop = new ForStmt(init, condition, update, body, line, col);
 * @endcode
 * 
 * Структура:
 * - init: инициализация (объявление переменной или выражение)
 * - condition: условие продолжения (если nullptr, то бесконечный цикл)
 * - update: выражение-обновление (выполняется после каждой итерации)
 * - body: тело цикла
 * 
 * @note Любая из частей (кроме body) может быть nullptr
 * @note Область видимости переменных из init ограничена циклом
 */
class ForStmt : public ASTNode {
public:
    ASTNode* init;          ///< Инициализация (VarDecl или ExprStmt, может быть nullptr)
    Expression* condition;  ///< Условие (может быть nullptr)
    Expression* update;     ///< Обновление (может быть nullptr)
    ASTNode* body;          ///< Тело цикла
    
    /**
     * @brief Конструктор цикла for
     * @param i Инициализация
     * @param cond Условие
     * @param upd Обновление
     * @param b Тело цикла
     * @param l Номер строки
     * @param c Номер колонки
     */
    ForStmt(ASTNode* i, Expression* cond, Expression* upd, ASTNode* b, int l, int c)
        : ASTNode(l, c), init(i), condition(cond), update(upd), body(b) {}
    
    /**
     * @brief Деструктор, освобождающий память
     */
    ~ForStmt() { delete init; delete condition; delete update; delete body; }
};

/**
 * @brief Узел AST, соответствующий оператору return
 * 
 * Оператор возврата из функции.
 * 
 * @code
 * ReturnStmt* ret = new ReturnStmt(expr, line, col);  // return expr;
 * ReturnStmt* retVoid = new ReturnStmt(nullptr, line, col);  // return;
 * @endcode
 * 
 * @note В функции с типом void выражение должно быть nullptr
 * @note В функции с типом, отличным от void, выражение обязательно
 */
class ReturnStmt : public ASTNode {
public:
    Expression* expr;   ///< Возвращаемое значение (может быть nullptr для void)
    
    /**
     * @brief Конструктор оператора return
     * @param e Выражение-результат (nullptr для void)
     * @param l Номер строки
     * @param c Номер колонки
     */
    ReturnStmt(Expression* e, int l, int c) 
        : ASTNode(l, c), expr(e) {}
    
    /**
     * @brief Деструктор, освобождающий память выражения
     */
    ~ReturnStmt() { delete expr; }
};

/**
 * @brief Узел AST, соответствующий оператору break
 * 
 * Оператор break осуществляет досрочный выход из цикла.
 * Может использоваться только внутри цикла (while, do-while, for).
 * 
 * @code
 * BreakStmt* br = new BreakStmt(line, col);
 * @endcode
 * 
 * @note Семантический анализатор проверяет, что break находится внутри цикла
 * @see WhileStmt, DoWhileStmt, ForStmt
 */
class BreakStmt : public ASTNode {
public:
    /**
     * @brief Конструктор оператора break
     * @param l Номер строки
     * @param c Номер колонки
     */
    BreakStmt(int l, int c) : ASTNode(l, c) {}
};

/**
 * @brief Узел AST, соответствующий оператору continue
 * 
 * Оператор continue осуществляет переход к следующей итерации цикла,
 * пропуская оставшуюся часть тела цикла.
 * 
 * @code
 * ContinueStmt* cont = new ContinueStmt(line, col);
 * @endcode
 * 
 * @note Семантический анализатор проверяет, что continue находится внутри цикла
 * @see WhileStmt, DoWhileStmt, ForStmt
 */
class ContinueStmt : public ASTNode {
public:
    /**
     * @brief Конструктор оператора continue
     * @param l Номер строки
     * @param c Номер колонки
     */
    ContinueStmt(int l, int c) : ASTNode(l, c) {}
};

/**
 * @brief Узел AST, соответствующий всей программе
 * 
 * Корневой узел AST. Содержит все глобальные объявления в программе:
 * - Объявления функций
 * - Глобальные переменные
 * 
 * @code
 * std::vector<ASTNode*> decls;
 * decls.push_back(func1);
 * decls.push_back(func2);
 * decls.push_back(globalVar);
 * Program* prog = new Program(decls, line, col);
 * @endcode
 * 
 * @note Все объявления на верхнем уровне считаются глобальными
 */
class Program : public ASTNode {
public:
    std::vector<ASTNode*> declarations;  ///< Список глобальных объявлений
    
    /**
     * @brief Конструктор программы
     * @param decls Вектор глобальных объявлений
     * @param l Номер строки
     * @param c Номер колонки
     */
    Program(const std::vector<ASTNode*>& decls, int l, int c) 
        : ASTNode(l, c), declarations(decls) {}
    
    /**
     * @brief Деструктор, освобождающий память всех объявлений
     */
    ~Program() { for (size_t i = 0; i < declarations.size(); ++i) delete declarations[i]; }
};

#endif