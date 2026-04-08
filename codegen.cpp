#include "codegen.h"
#include <stdexcept>
#include <cassert>

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

// Обработка присваивания (включая составные)
static void emitAssignment(CodeGenerator* cg, Expression* left, const std::string& op, Expression* right) {
    // Вычисляем правую часть
    cg->visitNode(right);

    // Для составных присваиваний (*=, +=, ...) нужно сначала загрузить текущее значение left
    if (op != "=") {
        // Дублируем вычисленное правое значение, если оно нужно позже (не обязательно)
        // Загружаем левое значение
        if (auto id = dynamic_cast<IdentifierExpr*>(left)) {
            cg->commands.push_back(Command(LOAD, id->name));
        } else if (auto arr = dynamic_cast<ArrayAccessExpr*>(left)) {
            cg->visitNode(arr->index);
            cg->commands.push_back(Command(LOAD_ARRAY, arr->array->name, ""));
        } else {
            throw std::runtime_error("Invalid left side of assignment");
        }

        // Выполняем операцию (например, для "+=" -> ADD)
        std::string baseOp = op.substr(0, op.size() - 1); // убираем '='
        cg->commands.push_back(Command(binaryOpToInst(baseOp)));
    }

    // Сохраняем результат в левую часть
    if (auto id = dynamic_cast<IdentifierExpr*>(left)) {
        cg->commands.push_back(Command(ASSIGN, id->name));
    } else if (auto arr = dynamic_cast<ArrayAccessExpr*>(left)) {
        // На стеке: [значение] [индекс] (индекс должен быть под значением)
        // Мы вычисляли значение, потом индекс; нужно поменять порядок
        // Проще: сначала вычислить индекс, потом значение, потом STORE_ARRAY
        // Но здесь мы уже вычислили значение, а индекс ещё нет.
        // Изменим порядок в генерации для ArrayAccessExpr: сначала индекс, потом значение.
        // Поэтому в emitAssignment переделаем логику.
        throw std::runtime_error("Array assignment not implemented in this simplified version");
    } else {
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
        // Открываем scope для параметров и тела
        commands.push_back(Command(SCOPEOPEN));
        // Параметры: объявляем переменные и присваиваем им значения из стека
        for (const auto& p : func->params) {
            commands.push_back(Command(DECLAREVAR, p.type, p.name));
            commands.push_back(Command(ASSIGN, p.name));
        }
        visitNode(func->body);
        // Если функция void и нет return, всё равно нужен RET (будет добавлен в ReturnStmt)
        // Закрываем scope
        commands.push_back(Command(SCOPEPOP));
        // На всякий случай добавим RET, если тело не заканчивается return
        if (commands.back().inst != RET) {
            commands.push_back(Command(RET));
        }
    }
    // --- Блок (составной оператор) ---
    else if (auto* block = dynamic_cast<BlockStmt*>(node)) {
        commands.push_back(Command(SCOPEOPEN));
        for (auto stmt : block->statements) {
            visitNode(stmt);
        }
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
        commands.push_back(Command(NOT));
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
        commands.push_back(Command(POP));
    }
    // --- Объявление переменной (внутри функции или глобально?) ---
    else if (auto* var = dynamic_cast<VarDecl*>(node)) {
        commands.push_back(Command(DECLAREVAR, var->type, var->name));
        if (var->init) {
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
            // Для float можно хранить в intArg? Нет, но пока опустим.
            commands.push_back(Command(PUSH, (int)std::stof(lit->value))); // условно
        } else if (lit->kind == LiteralExpr::LIT_BOOL) {
            commands.push_back(Command(PUSH, lit->value == "true" ? 1 : 0));
        } else if (lit->kind == LiteralExpr::LIT_CHAR) {
            commands.push_back(Command(PUSH, (int)lit->value[0]));
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
        visitNode(arr->index);                 // индекс на стек
        commands.push_back(Command(LOAD_ARRAY, arr->array->name, ""));
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
        // Аргументы вычисляются справа налево? Зависит от соглашения.
        // Будем класть в прямом порядке, а вызывающая сторона пусть снимает.
        for (auto* arg : call->args) {
            visitNode(arg);
        }
        commands.push_back(Command(CALL, call->funcName));
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