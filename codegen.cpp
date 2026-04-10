#include "codegen.h"
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <algorithm>

// ------------------------------------------------------------
//  Вспомогательные функции
// ------------------------------------------------------------

// Определить команду для бинарного оператора
static Instruction binaryOpToInst(const std::string& op) {
    if (op == "+")  return ADD;
    if (op == "-")  return SUB;
    if (op == "*")  return MUL;
    if (op == "/")  return DIV;
    if (op == "%")  return MOD;
    if (op == "==") return EQ;
    if (op == "!=") return NE;
    if (op == "<")  return LESS;
    if (op == ">")  return GREATER;
    if (op == "<=") return LE;
    if (op == ">=") return GE;
    if (op == "&&") return AND;
    if (op == "||") return OR;
    throw std::runtime_error("Unknown binary operator: " + op);
}

static void emitAssignment(CodeGenerator* cg, Expression* left, const std::string& op, Expression* right) {
    if (auto* id = dynamic_cast<IdentifierExpr*>(left)) {
        // Присваивание простой переменной
        cg->visitNode(right);
        if (op != "=") {
            cg->commands.push_back(Command(LOAD, id->name));
            std::string baseOp = op.substr(0, op.size() - 1);
            cg->commands.push_back(Command(binaryOpToInst(baseOp)));
        }
        cg->commands.push_back(Command(ASSIGN, id->name));
    }
    else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(left)) {
        // Присваивание элементу массива: arr[idx] = ...
        // Порядок: сначала индекс, потом значение
        cg->visitNode(arr->index);               // индекс на стек

        if (op != "=") {
            // Составное присваивание: arr[idx] += right
            cg->commands.push_back(Command(DUP));               // дублируем индекс
            cg->commands.push_back(Command(LOAD_ARRAY, arr->array->name)); // читаем старое значение
            // Стек: индекс, старое_значение
            cg->visitNode(right);                                // вычисляем правую часть
            // Стек: индекс, старое_значение, правое_значение
            std::string baseOp = op.substr(0, op.size() - 1);
            cg->commands.push_back(Command(binaryOpToInst(baseOp))); // операция
            // Стек: индекс, результат
        } else {
            // Обычное присваивание: arr[idx] = right
            cg->visitNode(right);
            // Стек: индекс, значение
        }
        cg->commands.push_back(Command(STORE_ARRAY, arr->array->name));
    }
    else {
        throw std::runtime_error("Invalid left side of assignment");
    }
}

// ------------------------------------------------------------
//  Генерация кода для узлов AST
// ------------------------------------------------------------
void CodeGenerator::visitNode(ASTNode* node) {
    if (!node) return;

    // --- Корень программы ---
    if (auto* program = dynamic_cast<Program*>(node)) {
        for (auto decl : program->declarations) {
            visitNode(decl);
        }
    }
    // --- Объявление функции ---
    else if (auto* func = dynamic_cast<FunctionDecl*>(node)) {
        functionStart[func->name] = commands.size();
        // Параметры объявляем в текущей области видимости
        commands.push_back(Command(SCOPEOPEN));
        for (const auto& p : func->params) {
            commands.push_back(Command(DECLAREVAR, p.type, p.name));
            commands.push_back(Command(ASSIGN, p.name));
        }
        openedScopes = 1;
        visitNode(func->body);
        
        // Гарантируем RET в конце
        if (commands.empty() || commands.back().inst != RET) {
            commands.push_back(SCOPEPOP);
            commands.push_back(Command(RET));
        }
    }
    // --- Блок (составной оператор) ---
    else if (auto* block = dynamic_cast<BlockStmt*>(node)) {
        openedScopes++;
        commands.push_back(Command(SCOPEOPEN));
        for (auto stmt : block->statements) {
            visitNode(stmt);
        }
        openedScopes--;
        commands.push_back(Command(SCOPEPOP));
    }
    // --- Условный оператор if ---
    else if (auto* ifs = dynamic_cast<IfStmt*>(node)) {
        visitNode(ifs->condition);
        commands.push_back(Command(NOT));
        size_t jumpToElse = emitPlaceholder(JUMPIFTRUE); // переход если false
        visitNode(ifs->thenBranch);
        // После then нужно перепрыгнуть else (если он есть)
        size_t jumpToEnd = 0;
        if (ifs->elseBranch) {
            jumpToEnd = emitPlaceholder(JUMP);
        }
        // Патчим переход на else (или на конец, если else нет)
        commands[jumpToElse] = Command(JUMPIFTRUE, (int)commands.size());
        if (ifs->elseBranch) {
            visitNode(ifs->elseBranch);
            commands[jumpToEnd] = Command(JUMP, (int)commands.size());
        }
    }
    // --- Цикл while ---
    else if (auto* wh = dynamic_cast<WhileStmt*>(node)) {
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t start = currentInstruction();
        visitNode(wh->condition);
        commands.push_back(Command(NOT));
        size_t jumpOut = emitPlaceholder(JUMPIFTRUE); // выход если false
        visitNode(wh->body);
        // Патчим continue (переход на проверку условия)
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)start);
        // Безусловный прыжок на начало цикла
        commands.push_back(Command(JUMP, (int)start));
        // Патчим break
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        commands[jumpOut] = Command(JUMPIFTRUE, (int)afterLoop);
        loopStack.pop_back();
    }
    // --- Цикл do-while ---
    else if (auto* dw = dynamic_cast<DoWhileStmt*>(node)) {
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t bodyStart = currentInstruction();
        visitNode(dw->body);
        // Патчим continue (переход на проверку условия)
        size_t condStart = currentInstruction();
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)condStart);
        visitNode(dw->condition);
        commands.push_back(Command(JUMPIFTRUE, (int)bodyStart)); // если false, повтор
        // Патчим break
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        loopStack.pop_back();
    }
    // --- Цикл for ---
    else if (auto* fr = dynamic_cast<ForStmt*>(node)) {
        commands.push_back(Command(SCOPEOPEN));
        if (fr->init) visitNode(fr->init);
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t condStart = currentInstruction();
        if (fr->condition) {
            visitNode(fr->condition);
        } else {
            // бесконечный цикл: условие true
            commands.push_back(Command(PUSH, 1)); // true
        }
        commands.push_back(Command(NOT));
        size_t jumpOut = emitPlaceholder(JUMPIFTRUE);
        visitNode(fr->body);
        // continue переходит на update
        size_t updateStart = currentInstruction();
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)updateStart);
        if (fr->update) visitNode(fr->update);
        commands.push_back(Command(JUMP, (int)condStart));
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        commands[jumpOut] = Command(JUMPIFTRUE, (int)afterLoop);
        commands.push_back(Command(SCOPEPOP));
        loopStack.pop_back();
    }
    // --- Оператор return ---
    else if (auto* ret = dynamic_cast<ReturnStmt*>(node)) {
        if (ret->expr) {
            visitNode(ret->expr);
        } else {
            // void: ничего не кладём
        }
        for (size_t i = 0; i < openedScopes; i++)
            commands.push_back(Command(SCOPEPOP));
        commands.push_back(Command(RET));
    }
    // --- break и continue ---
    else if (dynamic_cast<BreakStmt*>(node)) {
        if (loopStack.empty()) throw std::runtime_error("break outside loop");
        loopStack.back().breakAddr.push_back(currentInstruction());
        commands.push_back(Command()); // placeholder
    }
    else if (dynamic_cast<ContinueStmt*>(node)) {
        if (loopStack.empty()) throw std::runtime_error("continue outside loop");
        loopStack.back().continueAddr.push_back(currentInstruction());
        commands.push_back(Command()); // placeholder
    }
    // --- Выражение-оператор ---
    else if (auto* es = dynamic_cast<ExprStmt*>(node)) {
        visitNode(es->expr);
        // Результат выражения остаётся на стеке, но он не нужен — удаляем
        //commands.push_back(Command(POP));
    }
    // --- Объявление переменной (внутри функции или глобально?) ---
    else if (auto* var = dynamic_cast<VarDecl*>(node)) {
        commands.push_back(Command(DECLAREVAR, var->type, var->name));
        if (var->init) {
            if (var->type.isArray && var->type.base == TYPE_CHAR) {
                auto* lit = dynamic_cast<LiteralExpr*>(var->init);
                if (lit && lit->kind == LiteralExpr::LIT_STRING) {
                    // Кладём строку как аргумент команды (можно через argName)
                    commands.push_back(Command(INIT_ARRAY_STRING, var->name, lit->value));
                    // init уже обработан, выходим
                    return;
                }
            }
            // обычная инициализация скаляра
            visitNode(var->init);
            commands.push_back(Command(ASSIGN, var->name));
        }
    }
    // --- Литерал ---
    else if (auto* lit = dynamic_cast<LiteralExpr*>(node)) {
        // PUSH константы: нам нужен способ передать значение. Добавим команду PUSH с числом/строкой.
        // Для простоты будем использовать intArg или argName.
        if (lit->kind == LiteralExpr::LIT_INT) {
            commands.push_back(Command(PUSH, std::stoi(lit->value)));
        } else if (lit->kind == LiteralExpr::LIT_FLOAT) {
            commands.push_back(Command(PUSH, std::stof(lit->value)));
        } else if (lit->kind == LiteralExpr::LIT_BOOL) {
            commands.push_back(Command(PUSH, lit->value == "true" ? true : false));
        } else if (lit->kind == LiteralExpr::LIT_CHAR) {
            commands.push_back(Command(PUSH, lit->value[0]));
        } else {
            throw std::runtime_error("Unsupported literal kind");
        }
    }
    // --- Идентификатор (переменная) ---
    else if (auto* id = dynamic_cast<IdentifierExpr*>(node)) {
        commands.push_back(Command(LOAD, id->name));
    }
    // --- Доступ к массиву (чтение) ---
    else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        visitNode(arr->index);                 // индекс -> стек
        commands.push_back(Command(LOAD_ARRAY, arr->array->name));
    }
    // --- Унарное выражение ---
    else if (auto* un = dynamic_cast<UnaryExpr*>(node)) {
        visitNode(un->operand);
        if (un->op == "-") {
            commands.push_back(Command(NEG));
        } else if (un->op == "!") {
            commands.push_back(Command(NOT));
        } else if (un->op == "+") {
            // ничего не делаем
        } else {
            throw std::runtime_error("Unknown unary operator: " + un->op);
        }
    }
    // --- Бинарное выражение ---
    else if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
        // Проверяем, является ли оператор присваиванием
        if (bin->op == "=" || bin->op == "+=" || bin->op == "-=" ||
            bin->op == "*=" || bin->op == "/=" || bin->op == "%=") {
            emitAssignment(this, bin->left, bin->op, bin->right);
        } else {
            visitNode(bin->left);
            visitNode(bin->right);
            commands.push_back(Command(binaryOpToInst(bin->op)));
        }
    }
    // --- Вызов функции ---
    else if (auto* call = dynamic_cast<CallExpr*>(node)) {
        if (call->funcName == "read_int") {
            if (!call->args.empty())
                throw std::runtime_error("read_int takes no arguments");
            commands.push_back(Command(READ_INT));
        }
        else if (call->funcName == "read_float") {
            if (!call->args.empty())
                throw std::runtime_error("read_float takes no arguments");
            commands.push_back(Command(READ_FLOAT));
        }
        else if (call->funcName == "read_char") {
            if (!call->args.empty())
                throw std::runtime_error("read_char takes no arguments");
            commands.push_back(Command(READ_CHAR));
        } else if (call->funcName == "print_int") {
            if (call->args.size() != 1)
                throw std::runtime_error("print_int takes 1 argument");
            for (auto* arg : call->args)
                visitNode(arg);
            commands.push_back(Command(PRINT_INT));
        } else if (call->funcName == "print_char") {
            if (call->args.size() != 1)
                throw std::runtime_error("print_char takes 1 argument");
            for (auto* arg : call->args)
                visitNode(arg);
            commands.push_back(Command(PRINT_CHAR));
        }
        else if (call->funcName == "print_float") {
            if (call->args.size() != 1)
                throw std::runtime_error("print_float takes 1 argument");
            for (auto* arg : call->args)
                visitNode(arg);
            commands.push_back(Command(PRINT_FLOAT));
        } else {
            std::reverse(call->args.begin(), call->args.end());
            for (auto* arg : call->args)
                visitNode(arg);
            std::reverse(call->args.begin(), call->args.end());
            commands.push_back(Command(CALL, call->funcName));
        }
    }
    // --- Оператор "запятая" ---
    else if (auto* comma = dynamic_cast<CommaExpr*>(node)) {
        for (size_t i = 0; i < comma->exprs.size(); ++i) {
            visitNode(comma->exprs[i]);
            if (i != comma->exprs.size() - 1) {
                commands.push_back(Command(POP)); // убираем промежуточные значения
            }
        }
    }
    else {
        throw std::runtime_error("Unhandled AST node type in codegen");
    }
}

void CodeGenerator::generate() {
    std::vector<VarDecl*> globals;
    std::vector<FunctionDecl*> functions;

    for (auto decl : root->declarations) {
        if (auto* var = dynamic_cast<VarDecl*>(decl))
            globals.push_back(var);
        else if (auto* func = dynamic_cast<FunctionDecl*>(decl))
            functions.push_back(func);
    }

    // 1. Глобальный scope и объявления
    commands.push_back(Command(SCOPEOPEN));
    for (auto var : globals)
        commands.push_back(Command(DECLAREVAR, var->type, var->name));

    // 2. Инициализация глобалов
    for (auto var : globals) {
        if (var->init) {
            visitNode(var->init);
            commands.push_back(Command(ASSIGN, var->name));
        }
    }

    // 3. Вызов main и останов
    commands.push_back(Command(CALL, "main"));
    std::cout << commands.back().argName.size() << std::endl;
    std::cout << "------------------\n";
    commands.push_back(Command(POP));   // убрать результат main, если есть
    commands.push_back(Command(HALT));

    // 4. Генерируем функции (их код идёт после HALT, но адреса уже корректны)
    for (auto func : functions) {
        visitNode(func);   // здесь functionStart запишется с учётом размера commands
    }
}