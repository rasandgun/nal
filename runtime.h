#ifndef RUNTIME_H
#define RUNTIME_H
#include "codegen.h"
#include <stack>
#include <iostream>
#include <map>
#include <string>
#include <flat_map>

/**
 * @brief Печать значения
 * @param val Значение для печати
 */
inline void printValue(Value val) {
    if (val.type == TYPE_BOOL) {
        std::cout << val.bit;
    } else if (val.type == TYPE_CHAR){
        std::cout << val.byte;
    } else if (val.type == TYPE_INT){
        std::cout << val.i32;
    } else if (val.type == TYPE_FLOAT){
        std::cout << val.f64;
    } else  {
        std::cout << "UNKNOWN";
    }
}

/**
 * @brief Преобразование типа в строку
 * @param t Тип для преобразования
 * @return Строковое представление типа
 */
inline std::string typeToString(const Type& t) {
    std::string base;
    switch (t.base) {
        case TYPE_INT: base = "int"; break;
        case TYPE_CHAR: base = "char"; break;
        case TYPE_BOOL: base = "bool"; break;
        case TYPE_FLOAT: base = "float"; break;
        case TYPE_VOID: base = "void"; break;
        default: base = "unknown";
    }
    if (t.isArray) {
        base += "[" + std::to_string(t.size) + "]";
    }
    return base;
}

/**
 * @brief Печать команды виртуальной машины
 * @param cmd Команда для печати
 */
inline void printCommand(const Command& cmd) {
    switch (cmd.inst) {
        case ADD: std::cout << "ADD"; break;
        case SUB: std::cout << "SUB"; break;
        case MUL: std::cout << "MUL"; break;
        case DIV: std::cout << "DIV"; break;
        case MOD: std::cout << "MOD"; break;
        case NEG: std::cout << "NEG"; break;
        case GREATER: std::cout << "GREATER"; break;
        case LESS: std::cout << "LESS"; break;
        case GE: std::cout << "GE"; break;
        case LE: std::cout << "LE"; break;
        case EQ: std::cout << "EQ"; break;
        case NE: std::cout << "NE"; break;
        case AND: std::cout << "AND"; break;
        case OR: std::cout << "OR"; break;
        case NOT: std::cout << "NOT"; break;
        case POP: std::cout << "POP"; break;
        case PUSH:
            std::cout << "PUSH ";
            if (!cmd.argName.empty())
                std::cout << cmd.argName;
            else {
                printValue(cmd.argVal);
            }
            break;
        case LOAD:
            std::cout << "LOAD " << cmd.argName;
            break;
        case LOAD_ARRAY:
            std::cout << "LOAD_ARRAY " << cmd.argName;
            break;
        case STORE_ARRAY:
            std::cout << "STORE_ARRAY " << cmd.argName;
            break;
        case JUMP:
            std::cout << "JUMP " << cmd.argVal.i32;
            break;
        case JUMPIFTRUE:
            std::cout << "JUMPIFTRUE " << cmd.argVal.i32;
            break;
        case JUMPIFFALSE:
            std::cout << "JUMPIFFALSE " << cmd.argVal.i32;
            break;
        case SCOPEOPEN:
            std::cout << "SCOPEOPEN";
            break;
        case DECLAREVAR:
            std::cout << "DECLAREVAR " << cmd.argName << " " << typeToString(cmd.argType);
            break;
        case SCOPEPOP:
            std::cout << "SCOPEPOP";
            break;
        case CALL:
            std::cout << "CALL " << cmd.argName;
            break;
        case RET:
            std::cout << "RET";
            break;
        case ASSIGN:
            std::cout << "ASSIGN " << cmd.argName;
            break;
        case DUP        :  std::cout << "DUP";       break;
        case HALT       :  std::cout << "HALT";      break;
        case READ_INT   :  std::cout << "READ_INT";  break;
        case READ_FLOAT :  std::cout << "READ_FLOAT";break;
        case READ_CHAR  :  std::cout << "READ_CHAR"; break;
        case PRINT_INT:
          std::cout << "PRINT_INT";
          break;
        case PRINT_FLOAT  :  std::cout << "PRINT_FLOAT"; break;
        case PRINT_CHAR  :  std::cout << "PRINT_CHAR"; break;
        case INIT_ARRAY_STRING : std::cout << "INIT_ARRAY_STRING"; break;
        default:
            std::cout << cmd.inst << std::endl;
            std::cout << "UNKNOWN";
    }
    std::cout << std::endl;
}

/**
 * @brief Рекурсивная печать стека значений
 * @param s Стек значений
 */
inline void printRecursive(std::stack<Value>& s) {
    if (s.empty()) return;

    Value x = s.top();
    s.pop();
    
    printRecursive(s);
    printValue(x);
    std::cout << std::endl;
    s.push(x); 
}

/**
 * @brief Структура массива для виртуальной машины
 */
struct Array {
    BasicType type;
    size_t size;
    union {
        double *f64;
        int *i32;
        char *byte;
        bool *bit;
    };
    ~Array() {
        if (type == TYPE_BOOL)
            delete[] bit;
        if (type == TYPE_INT)
            delete[] i32;
        if (type == TYPE_FLOAT)
            delete[] f64;
        if (type == TYPE_CHAR)
            delete[] byte;
    }
};

/**
 * @brief Таблица идентификаторов с поддержкой вложенных областей видимости
 * @tparam T Тип хранимых данных
 */
template<typename T>
class TID {
public:
    /**
     * @brief Узел таблицы идентификаторов
     */
    struct node {
        node(node* prev) : prev(prev) {
        }
        node *prev;
        std::unordered_map<std::string, T> table;
    };
    
    /**
     * @brief Очистка узлов таблицы
     * @param v Начальный узел
     */
    void clear(node *v) {
        node *current = v;
        while (current != nullptr) {
            node *prev = current->prev;
            delete current;
            current = prev;
        }
    }
    
    TID() : top(nullptr), height(0) {
    }
    
    ~TID() {
        if (top)
            clear(top);
    }
    
    /**
     * @brief Открыть новую область видимости
     */
    void openScope() {
        height++;
        top = new node(top);
    }
    
    /**
     * @brief Закрыть текущую область видимости
     */
    void popScope() {
        height--;
        node *old = top;
        top = top->prev;
        delete old;
    }
    
    /**
     * @brief Поиск идентификатора
     * @param name Имя идентификатора
     * @return Указатель на найденное значение
     * @throws std::runtime_error Если идентификатор не найден
     */
    T* lookUp(const std::string &name) {
        node *current = top;
        while (current) {
            auto searchRes = current->table.find(name);
            if (searchRes != current->table.end())
                return &searchRes->second;
            current = current->prev;
        }
        throw std::runtime_error("Couldn't find symbol " + name);
    }
    
    /**
     * @brief Объявить идентификатор в текущей области
     * @param name Имя идентификатора
     * @throws std::runtime_error Если идентификатор уже объявлен
     */
    void declare(const std::string &name) {
        if (top->table.count(name))
            throw std::runtime_error(name + " was already declared at current scope");
        top->table[name] = T();
    }
    
    /**
     * @brief Получить глубину вложенности
     * @return Текущая глубина
     */
    size_t getHeight() const {
        return height;
    }
    
private:
    size_t height;
    node *top;
};

/**
 * @brief Исполнитель виртуальной машины
 */
class Runner {
public:
    /**
     * @brief Конструктор исполнителя
     * @param commands Список команд для выполнения
     * @param functionStart Карта начала функций
     */
    Runner(std::vector<Command> commands, std::unordered_map<std::string, size_t> functionStart) :
    commands_(std::move(commands)), functionStart_(std::move(functionStart)), current_command_(0){}
    
    /**
     * @brief Запуск выполнения программы
     */
    void run();
    
private:
    TID<Value> variableTID_;     ///< Таблица переменных
    TID<Array> arrayTID_;        ///< Таблица массивов
    std::vector<Command> commands_;      ///< Список команд
    std::unordered_map<std::string, size_t> functionStart_;  ///< Начала функций
    size_t current_command_;     ///< Текущий указатель команды
    std::stack<Value> stack_;    ///< Стек значений
    std::stack<size_t> callStack_; ///< Стек вызовов
    
    Value toFloat(Value x);
    Value toInt(Value x);
    Value toChar(Value x);
    Value toBool(Value x);
    
    void I_ADD();
    void I_SUB();
    void I_MUL();
    void I_DIV();
    void I_MOD();
    void I_NEG();
    void I_GREATER();
    void I_LESS();
    void I_GE();
    void I_LE();
    void I_EQ();
    void I_NE();
    void I_AND();
    void I_OR();
    void I_NOT();
    void I_POP();
    void I_PUSH(Value x);
    void I_JUMP(size_t pos);
    void I_JUMPIFTRUE(size_t pos);
    void I_JUMPIFFALSE(size_t pos);
    void I_LOAD(const std::string &name);
    void I_LOAD_ARRAY(const std::string &name);
    void I_STORE_ARRAY(const std::string &name);
    void I_ASSIGN(const std::string &name);
    void I_SCOPEOPEN();
    void I_DECLAREVAR(const std::string &name, Type type);
    void I_SCOPEPOP();
    void I_CALL(const std::string &name);
    void I_RET();
    void I_READ_INT();
    void I_READ_FLOAT();
    void I_READ_CHAR();
    void I_HALT();
    void I_PRINT_INT();
    void I_PRINT_CHAR();
    void I_PRINT_FLOAT();
    void I_INIT_ARRAY_STRING(const std::string& arrayName, const std::string& str);
};

#endif