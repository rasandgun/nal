#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <stdexcept>
#include <iostream>
#include "ast.h"
struct Value {
    BasicType type;
    union {
        double f64;
        int i32;
        char byte;
        bool bit;
    };
    Value(double x) : type(TYPE_FLOAT), f64(x) {}
    Value(int x)    : type(TYPE_INT),   i32(x) {}
    Value(char x)   : type(TYPE_CHAR), byte(x) {}
    Value(bool x)   : type(TYPE_BOOL),  bit(x) {} 
    Value() {}
};

// Расширенный набор инструкций для полноценной поддержки языка
enum Instruction {
    // Арифметика
    ADD,
    SUB,
    MUL,        // умножение
    DIV,
    MOD,
    NEG,        // унарный минус

    // Сравнения
    GREATER,
    LESS,
    GE,
    LE,
    EQ,
    NE,         // не равно (можно и через EQ+NOT, но удобнее явно)

    // Логические
    AND,        // логическое И
    OR,         // логическое ИЛИ
    NOT,

    // Стек и память
    POP,
    PUSH,       // положить константу / значение на стек
    LOAD,       // загрузить значение переменной на стек
    LOAD_ARRAY, // загрузить элемент массива (индекс на стеке)
    STORE_ARRAY,// сохранить в элемент массива (значение и индекс на стеке)

    // Переходы
    JUMP,
    JUMPIFTRUE,
    JUMPIFFALSE,// удобно для некоторых условий (опционально)

    // Области видимости / переменные
    SCOPEOPEN,
    DECLAREVAR,
    SCOPEPOP,

    // Функции
    CALL,
    RET,
    ASSIGN,     // присвоить значение со стека переменной
    DUP,            // дублировать вершину стека
    HALT,
    READ_INT,
    READ_FLOAT,
    READ_CHAR,
    PRINT_INT,
    PRINT_FLOAT,
    PRINT_CHAR,
    INIT_ARRAY_STRING,
};

// Команда виртуальной машины с гибкими параметрами
struct Command {
    Instruction inst;
    // Универсальные поля для хранения аргументов разных типов
    Type argType;               // тип для DECLAREVAR
    std::string argName;        // имя переменной/функции
    Value argVal;
    std::string argStr;
    // Конструкторы
    Command() : inst(POP), argVal(0) {}
    Command(Instruction i, const std::string& varName, const std::string& strValue)
    : inst(i), argName(varName), argStr(strValue) {}
    Command(Instruction i) : inst(i), argVal(0) {}
    Command(Instruction i, int val) : inst(i), argVal(val) {}
    Command(Instruction i, double val) : inst(i), argVal(val){}
    Command(Instruction i, char val) : inst(i), argVal(val){}
    Command(Instruction i, bool val) : inst(i), argVal(val){
    }
    Command(Instruction i, const std::string& name) : inst(i), argName(name), argVal(0) {
    }   
    Command(Instruction i, const char *name) : inst(i), argName(name), argVal(0) {
    }    
    Command(Instruction i, const Type& t, const std::string& name) : inst(i), argType(t), argName(name), argVal(0) {}
};

class CodeGenerator {
public:
    std::vector<Command> commands;

    CodeGenerator(Program* program) : root(program), openedScopes(0) {}

    void insertGlobalInitCall();

    void generate();

    void visitNode(ASTNode* node);

    size_t currentInstruction() { return commands.size(); }
    std::unordered_map<std::string, size_t> functionStart;

private:
    Program* root;
    
    struct LoopInfo {
        size_t startAddr;                   // адрес начала цикла (для continue)
        std::vector<size_t> breakAddr;      // адреса команд break (нужно заполнить JUMP)
        std::vector<size_t> continueAddr;   // адреса команд continue (нужно заполнить JUMP)
    };
    std::vector<LoopInfo> loopStack;
    size_t openedScopes;
    // Вспомогательный метод для вставки команды с последующей "заплаткой" адреса
    size_t emitPlaceholder(Instruction inst = JUMP) {
        commands.push_back(Command(inst, 0));
        return commands.size() - 1;
    }
};

#endif