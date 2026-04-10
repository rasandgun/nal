#ifndef RUNTIME_H
#define RUNTIME_H
#include "codegen.h"
#include <stack>
#include <iostream>

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
        case PRINT_INT  :  std::cout << "PRINT_INT"; break;
        case PRINT_FLOAT  :  std::cout << "PRINT_FLOAT"; break;
        case PRINT_CHAR  :  std::cout << "PRINT_CHAR"; break;
        
        default:
            std::cout << cmd.inst << std::endl;
            std::cout << "UNKNOWN";
    }
    std::cout << std::endl;
}


inline void printRecursive(std::stack<Value>& s) {
    if (s.empty()) return;

    Value x = s.top();
    s.pop();
    
    printRecursive(s); // Recurse to the bottom
    printValue(x);
    std::cout << std::endl;
    s.push(x); 
}


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
            delete bit;
        if (type == TYPE_INT)
            delete i32;
        if (type == TYPE_FLOAT)
            delete f64;
        if (type == TYPE_CHAR)
            delete byte;
    }
};
template<typename T>
class TID {
public:
    struct node {
        node(node* prev) : prev(prev) {
        }
        node *prev;
        std::unordered_map<std::string, T> table;
    };
    void clear(node *cur) {
        if (cur->prev) clear(cur->prev);
        delete cur;
    }
    TID() : top(nullptr) {
    }
    ~TID() {
        if (top)
            clear(top);
    }
    void openScope() {
        top = new node(top);
    }
    void popScope() {
        node *old = top;
        top = top->prev;
        delete old;
    }
    T* lookUp(const std::string &name) {
        node *current = top;
        while (current) {
            if (current->table.count(name))
                return &current->table[name];
            current = current->prev;
        }
        throw std::runtime_error("Couldn't find symbol " + name);
    }
    void declare(const std::string &name) {
        if (top->table.count(name))
            throw std::runtime_error(name + " was already declared at current scope");
        top->table[name] = T();
    }
private:
    node *top;
};

class Runner {
public:
    Runner(std::vector<Command> commands, std::unordered_map<std::string, size_t> functionStart) :
    commands_(std::move(commands)), functionStart_(std::move(functionStart)), current_command_(0){}
    void run();
private:
    TID<Value> variableTID_;
    TID<Array> arrayTID_;    
    std::vector<Command> commands_; 
    std::unordered_map<std::string, size_t> functionStart_;
    size_t current_command_;
    std::stack<Value> stack_;
    std::stack<size_t> callStack_;
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

