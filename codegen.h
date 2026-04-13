#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <stdexcept>
#include <iostream>
#include "ast.h"

/**
 * @brief Структура значения для виртуальной машины
 */
struct Value {
  BasicType type;
  union {
    double f64;
    int i32;
    char byte;
    bool bit;
  };
  
  Value(double x) : type(TYPE_FLOAT), f64(x) {}
  Value(int x) : type(TYPE_INT), i32(x) {}
  Value(char x) : type(TYPE_CHAR), byte(x) {}
  Value(bool x) : type(TYPE_BOOL), bit(x) {}
  Value() {}
};

/**
 * @brief Набор инструкций виртуальной машины
 */
enum Instruction {
    ADD,          ///< Сложение
    SUB,          ///< Вычитание
    MUL,          ///< Умножение
    DIV,          ///< Деление
    MOD,          ///< Остаток от деления
    NEG,          ///< Унарный минус
    GREATER,      ///< Больше
    LESS,         ///< Меньше
    GE,           ///< Больше или равно
    LE,           ///< Меньше или равно
    EQ,           ///< Равно
    NE,           ///< Не равно
    AND,          ///< Логическое И
    OR,           ///< Логическое ИЛИ
    NOT,          ///< Логическое НЕ
    POP,          ///< Удалить вершину стека
    PUSH,         ///< Поместить значение на стек
    LOAD,         ///< Загрузить значение переменной на стек
    LOAD_ARRAY,   ///< Загрузить элемент массива
    STORE_ARRAY,  ///< Сохранить в элемент массива
    JUMP,         ///< Безусловный переход
    JUMPIFTRUE,   ///< Переход если истина
    JUMPIFFALSE,  ///< Переход если ложь
    SCOPEOPEN,    ///< Открыть область видимости
    DECLAREVAR,   ///< Объявить переменную
    SCOPEPOP,     ///< Закрыть область видимости
    CALL,         ///< Вызов функции
    RET,          ///< Возврат из функции
    ASSIGN,       ///< Присваивание
    DUP,          ///< Дублировать вершину стека
    HALT,         ///< Остановка
    READ_INT,     ///< Чтение целого числа
    READ_FLOAT,   ///< Чтение вещественного числа
    READ_CHAR,    ///< Чтение символа
    PRINT_INT,    ///< Печать целого числа
    PRINT_FLOAT,  ///< Печать вещественного числа
    PRINT_CHAR,   ///< Печать символа
    INIT_ARRAY_STRING, ///< Инициализация строки-массива
};

/**
 * @brief Команда виртуальной машины
 */
struct Command {
    Instruction inst;
    Type argType;           ///< Тип аргумента для DECLAREVAR
    std::string argName;    ///< Имя переменной или функции
    Value argVal;           ///< Значение аргумента
    std::string argStr;     ///< Строковый аргумент
    
    Command() : inst(POP), argVal(0) {}
    Command(Instruction i, const std::string& varName, const std::string& strValue)
    : inst(i), argName(varName), argStr(strValue) {}
    Command(Instruction i) : inst(i), argVal(0) {}
    Command(Instruction i, int val) : inst(i), argVal(val) {}
    Command(Instruction i, double val) : inst(i), argVal(val){}
    Command(Instruction i, char val) : inst(i), argVal(val){}
    Command(Instruction i, bool val) : inst(i), argVal(val){}
    Command(Instruction i, const std::string& name) : inst(i), argName(name), argVal(0) {}   
    Command(Instruction i, const char *name) : inst(i), argName(name), argVal(0) {}    
    Command(Instruction i, const Type& t, const std::string& name) : inst(i), argType(t), argName(name), argVal(0) {}
};

/**
 * @brief Генератор кода для виртуальной машины
 */
class CodeGenerator {
public:
    std::vector<Command> commands;  ///< Сгенерированные команды

    /**
     * @brief Конструктор
     * @param program Корневой узел AST программы
     */
    CodeGenerator(Program* program) : root(program), openedScopes(0) {}

    /**
     * @brief Вставить вызов глобальной инициализации
     */
    void insertGlobalInitCall();

    /**
     * @brief Запустить генерацию кода
     */
    void generate();

    /**
     * @brief Посетить узел AST
     * @param node Узел AST
     */
    void visitNode(ASTNode* node);

    /**
     * @brief Получить текущую позицию в списке команд
     * @return Индекс текущей команды
     */
    size_t currentInstruction() { return commands.size(); }
    
    std::unordered_map<std::string, size_t> functionStart;  ///< Карта начала функций

private:
    Program* root;  ///< Корень AST
    
    /**
     * @brief Информация о цикле для continue/break
     */
    struct LoopInfo {
        size_t startAddr;                   ///< Адрес начала цикла
        std::vector<size_t> breakAddr;      ///< Адреса break для заполнения
        std::vector<size_t> continueAddr;   ///< Адреса continue для заполнения
    };
    
    std::vector<LoopInfo> loopStack;  ///< Стек информации о циклах
    size_t openedScopes;              ///< Счетчик открытых областей видимости

    /**
     * @brief Сгенерировать команду-заглушку
     * @param inst Инструкция
     * @return Индекс сгенерированной команды
     */
    size_t emitPlaceholder(Instruction inst) {
        commands.push_back(Command(inst, 0));
        return commands.size() - 1;
    }
    
    /**
     * @brief Сгенерировать код присваивания
     * @param left Левая часть присваивания
     * @param op Оператор присваивания
     * @param right Правая часть присваивания
     */
    void emitAssignment(Expression* left, const std::string& op, Expression* right);
};

#endif