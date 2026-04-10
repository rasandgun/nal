#include "runtime.h"
#include <cmath>
#include <string>

Value Runner::toFloat(Value x) {
    switch (x.type) {
    case TYPE_FLOAT:
        return x;
    case TYPE_INT:
        return Value((double)x.i32);
    case TYPE_CHAR:
        return Value((double)x.byte);
    case TYPE_BOOL:
        return Value((double)x.bit);
    case TYPE_VOID:
        throw std::runtime_error("Trying to cast VOID to FLOAT");
    default:
        throw std::runtime_error("Trying to cast UNKNOWN TYPE to FLOAT");
    }
}

Value Runner::toInt(Value x) {
    switch (x.type) {
    case TYPE_FLOAT:
        return Value((int)x.f64);
    case TYPE_INT:
        return x;
    case TYPE_CHAR:
        return Value((int)x.byte);
    case TYPE_BOOL:
        return Value((int)x.bit);
    case TYPE_VOID:
        throw std::runtime_error("Trying to cast VOID to INT");
    default:
        throw std::runtime_error("Trying to cast UNKNOWN TYPE to INT");
    }
}

Value Runner::toChar(Value x) {
    switch (x.type) {
    case TYPE_FLOAT:
        return Value((char)x.f64);
    case TYPE_INT:
        return Value((char)x.i32);
    case TYPE_CHAR:
        return Value(x.byte);
    case TYPE_BOOL:
        return Value((char)x.bit);
    case TYPE_VOID:
        throw std::runtime_error("Trying to cast VOID to CHAR");
    default:
        throw std::runtime_error("Trying to cast UNKNOWN TYPE to BOOL");
    }
}

Value Runner::toBool(Value x) {
    switch (x.type) {
    case TYPE_FLOAT:
        return Value((bool)x.f64);
    case TYPE_INT:
        return Value((bool)x.i32);
    case TYPE_CHAR:
        return Value((bool)x.byte);
    case TYPE_BOOL:
        return Value(x.bit);
    case TYPE_VOID:
        throw std::runtime_error("Trying to cast VOID to CHAR");
    default:
        throw std::runtime_error("Trying to cast UNKNOWN TYPE to BOOL");
    }
}

void Runner::I_ADD() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform ADD");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 + toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 + toInt(rhs).i32);
}

void Runner::I_SUB() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform SUB" + std::to_string(current_command_));
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 - toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 - toInt(rhs).i32);
}

void Runner::I_MUL() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform MUL");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 * toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 * toInt(rhs).i32);
}

void Runner::I_DIV() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform MUL");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 / toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 / toInt(rhs).i32);
}

void Runner::I_MOD() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform MUL");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(fmod(toFloat(lhs).f64, toFloat(rhs).f64));
        return;
    }
    stack_.push(toInt(lhs).i32 % toInt(rhs).i32);
}

void Runner::I_NEG() {
    if (stack_.size() < 1) throw std::runtime_error("Stack does not have enough values to perform MUL");
    Value x = stack_.top();
    stack_.pop();
    switch (x.type) {
    case TYPE_FLOAT:
        stack_.push(-x.f64);
        break;
    case TYPE_INT:
        stack_.push(-x.i32);
        break;
    case TYPE_CHAR:
        stack_.push(-x.byte);
        break;
    case TYPE_BOOL:
        throw std::runtime_error("Trying to perform unary minus to a bool value");
    case TYPE_VOID:
        throw std::runtime_error("Trying to perform unary minus to a void value");
    }
}

void Runner::I_GREATER() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform GREATER");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 > toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 > toInt(rhs).i32);
}

void Runner::I_GE() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform GE");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 >= toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 >= toInt(rhs).i32);
}

void Runner::I_LESS() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform LESS");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 < toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 < toInt(rhs).i32);
}

void Runner::I_LE() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform LE");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 <= toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 <= toInt(rhs).i32);
}

void Runner::I_EQ() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform EQ");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 == toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 == toInt(rhs).i32);
}

void Runner::I_NE() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform NE");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type == TYPE_FLOAT || lhs.type == TYPE_FLOAT) {
        stack_.push(toFloat(lhs).f64 != toFloat(rhs).f64);
        return;
    }
    stack_.push(toInt(lhs).i32 != toInt(rhs).i32);
}


void Runner::I_AND() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform NE");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type != TYPE_BOOL || lhs.type != TYPE_BOOL) throw std::runtime_error("Trying to AND non-bool values");
    stack_.push(lhs.bit && rhs.bit);
}

void Runner::I_OR() {
    if (stack_.size() < 2) throw std::runtime_error("Stack does not have enough values to perform OR");
    Value rhs = stack_.top();
    stack_.pop();
    Value lhs = stack_.top();
    stack_.pop();
    if (rhs.type != TYPE_BOOL || lhs.type != TYPE_BOOL) throw std::runtime_error("Trying to OR non-bool values");
    stack_.push(lhs.bit || rhs.bit);
}

void Runner::I_NOT() {
    if (stack_.size() < 1) throw std::runtime_error("Stack does not have enough values to perform NOT");
    Value x = stack_.top();
    stack_.pop();
    if (x.type != TYPE_BOOL) throw std::runtime_error("Trying to NOT a non-bool value");
    stack_.push(!x.bit);
}

void Runner::I_POP() {
    if (stack_.size() < 1) throw std::runtime_error("Stack does not have enough values to perform POP at" + std::to_string(current_command_));
    stack_.pop();
}

void Runner::I_PUSH(Value x) {
    stack_.push(x);
}

void Runner::I_JUMP(size_t pos) {
    current_command_ = pos;
}

void Runner::I_SCOPEOPEN() {
    variableTID_.openScope();
    arrayTID_.openScope();
}

void Runner::I_DECLAREVAR(const std::string &name, Type type) {
    if (type.isArray) {
        ////TODO: 
        //throw std::runtime_error("Arrays are not implemented yet");
        arrayTID_.declare(name);
        auto& arr = *arrayTID_.lookUp(name);
        arr.type = type.base;
        arr.size = type.size;
        switch (type.base)
        {
            case TYPE_FLOAT:
                arr.f64 = new double[arr.size];
                break;
            case TYPE_CHAR:
                arr.byte = new char[arr.size];
                break;
            case TYPE_BOOL:
                arr.bit = new bool[arr.size];
                break;
            case TYPE_INT:
                arr.i32 = new int[arr.size];
                break;
            case TYPE_VOID:
                throw std::runtime_error("Can't declare array of type VOID");
            default:
                throw std::runtime_error("Unknown type of array");
        }
    }
    variableTID_.declare(name);
    auto &var = *variableTID_.lookUp(name);
    var.type = type.base;
}

void Runner::I_SCOPEPOP() {
    variableTID_.popScope();
    arrayTID_.popScope();
}

void Runner::I_LOAD(const std::string &name) {
    stack_.push(*variableTID_.lookUp(name));
}

void Runner::I_ASSIGN(const std::string &name) {
    auto& var = *variableTID_.lookUp(name);
    switch(var.type) {
        case TYPE_BOOL:
            var = toBool(stack_.top());
            break;
        case TYPE_CHAR:
            var = toChar(stack_.top());
            break;
        case TYPE_INT:
            var = toInt(stack_.top());
            break;
        case TYPE_FLOAT:
            var = toFloat(stack_.top());
            break;
        case TYPE_VOID:
            throw std::runtime_error("Trying to assign void variable something\n");
            break;
        default:
            throw std::runtime_error("Found unknown type");
            break;
    }
    stack_.pop();
}

void Runner::I_CALL(const std::string &name) {
    if (!functionStart_.count(name))
        throw std::runtime_error("Couldn't find function" + name) ;
    callStack_.push(current_command_ + 1);
    current_command_ = functionStart_[name];
}

void Runner::I_RET() {
    if (callStack_.empty())
        throw std::runtime_error("callStack is empty");
    current_command_ = callStack_.top();
    callStack_.pop();
}

void Runner::I_HALT() {

}

void Runner::I_READ_INT() {
    int x;
    std::cin >> x;
    stack_.push(x);
}

void Runner::I_PRINT_INT() {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    if (stack_.top().type != TYPE_INT)
        throw std::runtime_error("Stack top is not int");
    std::cout << stack_.top().i32;
    std::cout.flush();
    stack_.pop(); 
}

void Runner::I_JUMPIFTRUE(size_t pos) {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    if (toBool(stack_.top()).bit) {
        current_command_ = pos;
    } else
        current_command_++;
    stack_.pop();
}

void Runner::I_JUMPIFFALSE(size_t pos) {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    if (!toBool(stack_.top()).bit) {
        current_command_ = pos;
    } else
        current_command_++;
    stack_.pop();
}

void Runner::I_LOAD_ARRAY(const std::string &name) {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    const auto &arr = *arrayTID_.lookUp(name);
    size_t index = toInt(stack_.top()).i32;
    if (index >= arr.size)
        throw std::out_of_range("Array " + name + " index " + std::to_string(index) + "out of range");
    stack_.pop();
    switch (arr.type)
    {
    case TYPE_INT:
        stack_.push(arr.i32[index]);
        break;
    case TYPE_BOOL:
        stack_.push(arr.bit[index]);
        break;
    case TYPE_CHAR:
        stack_.push(arr.byte[index]);
        break;
    case TYPE_FLOAT:
        stack_.push(arr.f64[index]);
        break;
    default:
        throw std::runtime_error("Unknown type or void");
        break;
    }
}

    
void Runner::I_STORE_ARRAY(const std::string &name) {
    if (stack_.size() < 2) throw std::runtime_error("Not enough elements to perform STORE_ARRAY on a stack");
    Value val = stack_.top();
    stack_.pop();
    size_t index = toInt(stack_.top()).i32;
    stack_.pop();
    auto &arr = *arrayTID_.lookUp(name);
    switch(arr.type) {
        case TYPE_INT:
            arr.i32[index] = toInt(val).i32;
            break;
        case TYPE_BOOL:
            arr.bit[index] = toBool(val).bit;
            break;
        case TYPE_CHAR:
            arr.byte[index] = toChar(val).byte;
            break;
        case TYPE_FLOAT:
            arr.f64[index] = toFloat(val).f64;
            break;
        default:
            std::runtime_error("Unknown type or void");
    }
}

void Runner::I_READ_FLOAT() {
    double x;
    std::cin >> x;
    stack_.push(x);
}

void Runner::I_READ_CHAR() {
    char c;
    std::cin >> c;
    stack_.push(c);
}
    
void Runner::I_PRINT_CHAR() {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    std::cout << toChar(stack_.top()).byte;
}

void Runner::I_PRINT_FLOAT() {
    if (stack_.empty())
        throw std::runtime_error("Stack is empty");
    std::cout << toFloat(stack_.top()).f64;
}
    

void Runner::I_INIT_ARRAY_STRING(const std::string& arrayName, const std::string& str) {
    auto* arr = arrayTID_.lookUp(arrayName);
    if (!arr) throw std::runtime_error("Array not found: " + arrayName);
    if (arr->type != TYPE_CHAR)
        throw std::runtime_error("INIT_ARRAY_STRING can only be used on char array");

    size_t len = str.length();
    if (len > arr->size)
        throw std::runtime_error("String literal too long for array " + arrayName);

    for (size_t i = 0; i < len; ++i)
        arr->byte[i] = str[i];
    // остаток массива можно не обнулять
}

void Runner::run() {
    while (commands_[current_command_].inst != HALT) {
        auto &command = commands_[current_command_];
        //printCommand(command);
        //std::cout << std::endl;
        switch (command.inst)
        {
        case SCOPEOPEN:
            I_SCOPEOPEN();
            current_command_++;
            break;
        case CALL:
            I_CALL(command.argName);
            break;
        case POP:
            I_POP();
            current_command_++;
            break;
        case HALT:
            I_HALT();
            current_command_++;
            break;
        case DECLAREVAR:
            I_DECLAREVAR(command.argName, command.argType);
            current_command_++;
            break;
        case ASSIGN:
            I_ASSIGN(command.argName);
            current_command_++;
            break;
        case LOAD:
            I_LOAD(command.argName);
            current_command_++;
            break;
        case EQ:
            I_EQ();
            current_command_++;
            break;
        case NOT:
            I_NOT();
            current_command_++;
            break;
        case MOD:
            I_MOD();
            current_command_++;
            break;
        case MUL:
            I_MUL();
            current_command_++;
            break;
        case SUB:
            I_SUB();
            current_command_++;
            break;
        case DIV:
            I_DIV();
            current_command_++;
            break;
        case NEG:
            I_NEG();
            current_command_++;
            break;
        case GREATER:
            I_GREATER();
            current_command_++;
            break;
        case RET:
            I_RET();
            break;
        case READ_INT:
            I_READ_INT();
            current_command_++;
            break;
        case PUSH:
            I_PUSH(command.argVal);
            current_command_++;
            break;
        case PRINT_INT:
            I_PRINT_INT();
            current_command_++;
            break;
        case JUMPIFTRUE:
            I_JUMPIFTRUE(command.argVal.i32);
            break;
        case PRINT_FLOAT:
            I_PRINT_FLOAT();
            current_command_++;
            break;
        case PRINT_CHAR:
            I_PRINT_CHAR();
            current_command_++;
            break;
        case READ_CHAR:
            I_READ_CHAR();
            current_command_++;
            break;
        case READ_FLOAT:
            I_READ_FLOAT();
            current_command_++;
            break;
        case STORE_ARRAY:
            I_STORE_ARRAY(command.argName);
            current_command_++;
            break;
        case LOAD_ARRAY:
            I_LOAD_ARRAY(command.argName);
            current_command_++;
            break;
        case JUMPIFFALSE:
            I_JUMPIFFALSE(command.argVal.i32);
            break;
        case LESS:
            I_LESS();
            current_command_++;
            break;
        case SCOPEPOP:
            I_SCOPEPOP();
            current_command_++;
            break;
        case ADD:
            I_ADD();
            current_command_++;
            break;
        case JUMP:
            I_JUMP(command.argVal.i32);
            break;
        case INIT_ARRAY_STRING:
            I_INIT_ARRAY_STRING(command.argName, command.argStr);
            current_command_++;
            break;
        default:
            std::cout << "FAILED" << std::endl;
            std::cout << command.inst;
            throw std::runtime_error("aa");
            break;
        }
        //printRecursive(stack_);
    }
}

