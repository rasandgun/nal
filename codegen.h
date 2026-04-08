#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <stdexcept>
#include "ast.h"

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
};

// Команда виртуальной машины с гибкими параметрами
struct Command {
    Instruction inst;
    // Универсальные поля для хранения аргументов разных типов
    Type argType;               // тип для DECLAREVAR
    std::string argName;        // имя переменной/функции
    int intArg;                 // целочисленный аргумент (смещение, размер)
    std::string strArg2;        // дополнительная строка (например, для массива)

    // Конструкторы
    Command() : inst(POP), intArg(0) {}
    Command(Instruction i) : inst(i), intArg(0) {}
    Command(Instruction i, int val) : inst(i), intArg(val) {}
    Command(Instruction i, const std::string& name) : inst(i), argName(name), intArg(0) {}
    Command(Instruction i, const Type& t, const std::string& name) : inst(i), argType(t), argName(name), intArg(0) {}
    // Для работы с массивами: имя массива + возможно что-то ещё
    Command(Instruction i, const std::string& arrName, const std::string&) : inst(i), argName(arrName), intArg(0) {}
};

class CodeGenerator {
public:
    std::vector<Command> commands;

    CodeGenerator(Program* program) : root(program) {}

    void generate() {
        visitNode(root);
    }

    void visitNode(ASTNode* node);

    size_t currentInstruction() { return commands.size(); }

private:
    Program* root;
    std::unordered_map<std::string, size_t> functionStart; // начало функции (адрес первой инструкции)

    struct LoopInfo {
        size_t startAddr;                   // адрес начала цикла (для continue)
        std::vector<size_t> breakAddr;      // адреса команд break (нужно заполнить JUMP)
        std::vector<size_t> continueAddr;   // адреса команд continue (нужно заполнить JUMP)
    };
    std::vector<LoopInfo> loopStack;

    // Вспомогательный метод для вставки команды с последующей "заплаткой" адреса
    size_t emitPlaceholder(Instruction inst = JUMP) {
        commands.push_back(Command(inst, 0));
        return commands.size() - 1;
    }
};

#endif