#include "semantic.h"
#include <stdexcept>
#include <map>

// ----- Scope -----
SemanticAnalyzer::Scope::Scope(Scope* p) : parent(p) {}

SemanticAnalyzer::Scope::~Scope() {
    // не удаляем parent, он управляется внешне
}

void SemanticAnalyzer::Scope::declare(const std::string& name, const Type& type, bool isFunc, const std::vector<Parameter>& params) {
    if (symbols.find(name) != symbols.end())
        throw std::runtime_error("Symbol already declared in this scope: " + name);
    Symbol sym;
    sym.type = type;
    sym.initialized = false;
    sym.isFunction = isFunc;
    sym.params = params;
    symbols[name] = sym;
}

void SemanticAnalyzer::Scope::define(const std::string& name) {
    Symbol* s = lookup(name);
    if (s) s->initialized = true;
    else throw std::runtime_error("Symbol not found: " + name);
}

SemanticAnalyzer::Symbol* SemanticAnalyzer::Scope::lookup(const std::string& name) {
    std::map<std::string, Symbol>::iterator it = symbols.find(name);
    if (it != symbols.end()) return &it->second;
    if (parent) return parent->lookup(name);
    return NULL;
}

bool SemanticAnalyzer::Scope::isInitialized(const std::string& name) {
    Symbol* s = lookup(name);
    return s && s->initialized;
}

Type* SemanticAnalyzer::Scope::getType(const std::string& name) {
    Symbol* s = lookup(name);
    if (s) return &s->type;
    return NULL;
}

bool SemanticAnalyzer::Scope::isFunction(const std::string& name) {
    Symbol* s = lookup(name);
    return s && s->isFunction;
}

std::vector<Parameter>* SemanticAnalyzer::Scope::getParams(const std::string& name) {
    Symbol* s = lookup(name);
    if (s && s->isFunction) return &s->params;
    return NULL;
}

SemanticAnalyzer::Scope* SemanticAnalyzer::Scope::pushScope() {
    return new Scope(this);
}

SemanticAnalyzer::Scope* SemanticAnalyzer::Scope::popScope() {
    Scope* old = this;
    Scope* newCurrent = parent;
    delete old;
    return newCurrent;
}

// ----- SemanticAnalyzer -----
SemanticAnalyzer::SemanticAnalyzer() {
    global = new Scope();
    current = global;
    inLoop = false;
}

void SemanticAnalyzer::analyze(Program* program) {
    // Сначала собираем все объявления функций (и глобальных переменных)
    collectDeclarations(program);
    // Затем анализируем тела
    analyzeNode(program);
}

void SemanticAnalyzer::collectDeclarations(Program* prog) {
    for (size_t i = 0; i < prog->declarations.size(); ++i) {
        ASTNode* node = prog->declarations[i];
        if (FunctionDecl* func = dynamic_cast<FunctionDecl*>(node)) {
            global->declare(func->name, func->returnType, true, func->params);
        } else if (VarDecl* var = dynamic_cast<VarDecl*>(node)) {
            global->declare(var->name, var->type, false);
        }
    }
}

void SemanticAnalyzer::analyzeNode(ASTNode* node) {
    if (!node) return;
    if (Program* prog = dynamic_cast<Program*>(node)) {
        for (size_t i = 0; i < prog->declarations.size(); ++i) {
            analyzeNode(prog->declarations[i]);
        }
    } else if (FunctionDecl* func = dynamic_cast<FunctionDecl*>(node)) {
        // Создаём новую область
        Scope* prev = current;
        current = current->pushScope();
        // Добавляем параметры
        for (size_t i = 0; i < func->params.size(); ++i) {
            Parameter& p = func->params[i];
            current->declare(p.name, p.type);
            current->define(p.name); // параметры инициализированы
        }
        Type prevRet = currentReturnType;
        currentReturnType = func->returnType;
        bool prevLoop = inLoop;
        inLoop = false;
        // Анализируем тело
        analyzeNode(func->body);
        // Восстанавливаем
        currentReturnType = prevRet;
        inLoop = prevLoop;
        current = current->popScope();
    } else if (VarDecl* var = dynamic_cast<VarDecl*>(node)) {
        if (current->lookup(var->name) != NULL) {
            throw std::runtime_error("Variable already declared: " + var->name);
        }
        current->declare(var->name, var->type);
        if (var->init) {
            Type initType = getExprType(var->init);
            if (!typeCompatible(var->type, initType)) {
                throw std::runtime_error("Type mismatch in initialization of " + var->name);
            }
            current->define(var->name);
        }
    } else if (BlockStmt* block = dynamic_cast<BlockStmt*>(node)) {
        Scope* prev = current;
        current = current->pushScope();
        for (size_t i = 0; i < block->statements.size(); ++i) {
            analyzeNode(block->statements[i]);
        }
        current = current->popScope();
    } else if (IfStmt* ifs = dynamic_cast<IfStmt*>(node)) {
        Type condType = getExprType(ifs->condition);
        if (!isScalar(condType)) {
            throw std::runtime_error("Condition must be scalar");
        }
        analyzeNode(ifs->thenBranch);
        if (ifs->elseBranch) analyzeNode(ifs->elseBranch);
    } else if (WhileStmt* wh = dynamic_cast<WhileStmt*>(node)) {
        Type condType = getExprType(wh->condition);
        if (!isScalar(condType)) throw std::runtime_error("Condition must be scalar");
        bool prevLoop = inLoop;
        inLoop = true;
        analyzeNode(wh->body);
        inLoop = prevLoop;
    } else if (DoWhileStmt* dw = dynamic_cast<DoWhileStmt*>(node)) {
        bool prevLoop = inLoop;
        inLoop = true;
        analyzeNode(dw->body);
        inLoop = prevLoop;
        Type condType = getExprType(dw->condition);
        if (!isScalar(condType)) throw std::runtime_error("Condition must be scalar");
    } else if (ForStmt* fr = dynamic_cast<ForStmt*>(node)) {
        if (fr->init) analyzeNode(fr->init);
        if (fr->condition) {
            Type condType = getExprType(fr->condition);
            if (!isScalar(condType)) throw std::runtime_error("Condition must be scalar");
        }
        if (fr->update) getExprType(fr->update);
        bool prevLoop = inLoop;
        inLoop = true;
        analyzeNode(fr->body);
        inLoop = prevLoop;
    } else if (ReturnStmt* ret = dynamic_cast<ReturnStmt*>(node)) {
        if (currentReturnType.base == TYPE_VOID) {
            if (ret->expr) throw std::runtime_error("Return with value in void function");
        } else {
            if (!ret->expr) throw std::runtime_error("Return without value in non-void function");
            Type exprType = getExprType(ret->expr);
            if (!typeCompatible(currentReturnType, exprType))
                throw std::runtime_error("Return type mismatch");
        }
    } else if (BreakStmt* br = dynamic_cast<BreakStmt*>(node)) {
        if (!inLoop) throw std::runtime_error("Break outside loop");
    } else if (ContinueStmt* cont = dynamic_cast<ContinueStmt*>(node)) {
        if (!inLoop) throw std::runtime_error("Continue outside loop");
    } else if (ExprStmt* es = dynamic_cast<ExprStmt*>(node)) {
        if (es->expr) getExprType(es->expr);
    } else if (BinaryExpr* bin = dynamic_cast<BinaryExpr*>(node)) {
        getExprType(bin); // для побочных эффектов (проверка типов)
    } else if (UnaryExpr* un = dynamic_cast<UnaryExpr*>(node)) {
        getExprType(un);
    } else if (CommaExpr* comm = dynamic_cast<CommaExpr*>(node)) {
        for (size_t i = 0; i < comm->exprs.size(); ++i) {
            getExprType(comm->exprs[i]);
        }
    } else if (CallExpr* call = dynamic_cast<CallExpr*>(node)) {
        getExprType(call);
    } else if (ArrayAccessExpr* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        getExprType(arr);
    } else if (IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(node)) {
        getExprType(id);
    } else if (LiteralExpr* lit = dynamic_cast<LiteralExpr*>(node)) {
        // nothing
    } else {
        // unknown node
    }
}

Type SemanticAnalyzer::getExprType(ASTNode* node) {
    if (LiteralExpr* lit = dynamic_cast<LiteralExpr*>(node)) {
        switch (lit->kind) {
            case LiteralExpr::LIT_INT: return Type(TYPE_INT);
            case LiteralExpr::LIT_FLOAT: return Type(TYPE_FLOAT);
            case LiteralExpr::LIT_CHAR: return Type(TYPE_CHAR);
            case LiteralExpr::LIT_STRING: return Type(TYPE_CHAR, true, 0);
            case LiteralExpr::LIT_BOOL: return Type(TYPE_BOOL);
        }
    } else if (IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(node)) {
        Symbol* sym = current->lookup(id->name);
        if (!sym) throw std::runtime_error("Undeclared identifier: " + id->name);
        if (!sym->initialized && !sym->isFunction)
            throw std::runtime_error("Variable used before initialization: " + id->name);
        return sym->type;
    } else if (BinaryExpr* bin = dynamic_cast<BinaryExpr*>(node)) {
        Type left = getExprType(bin->left);
        Type right = getExprType(bin->right);
        std::string op = bin->op;
        if (op == "=" || op == "*=" || op == "/=" || op == "%=" || op == "+=" || op == "-=") {
            // Проверка lvalue: левая часть должна быть идентификатором или элементом массива
            if (!dynamic_cast<IdentifierExpr*>(bin->left) && !dynamic_cast<ArrayAccessExpr*>(bin->left))
                throw std::runtime_error("Left side of assignment must be lvalue");
            if (!typeCompatible(left, right))
                throw std::runtime_error("Type mismatch in assignment");
            return left;
        } else if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (!isArithmetic(left) || !isArithmetic(right))
                throw std::runtime_error("Arithmetic operation requires arithmetic types");
            if (left.base == TYPE_FLOAT || right.base == TYPE_FLOAT)
                return Type(TYPE_FLOAT);
            else
                return Type(TYPE_INT);
        } else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            if (!typeCompatible(left, right))
                throw std::runtime_error("Comparison of incompatible types");
            return Type(TYPE_BOOL);
        } else if (op == "&&" || op == "||") {
            if (!isScalar(left) || !isScalar(right))
                throw std::runtime_error("Logical operation requires scalar types");
            return Type(TYPE_BOOL);
        } else {
            throw std::runtime_error("Unknown operator: " + op);
        }
    } else if (UnaryExpr* un = dynamic_cast<UnaryExpr*>(node)) {
        Type opnd = getExprType(un->operand);
        if (un->op == "+" || un->op == "-") {
            if (!isArithmetic(opnd))
                throw std::runtime_error("Unary +/- requires arithmetic type");
            return opnd;
        } else if (un->op == "!") {
            if (!isScalar(opnd))
                throw std::runtime_error("Unary ! requires scalar type");
            return Type(TYPE_BOOL);
        } else {
            throw std::runtime_error("Unknown unary operator");
        }
    } else if (CallExpr* call = dynamic_cast<CallExpr*>(node)) {
        Symbol* sym = current->lookup(call->funcName);
        if (!sym || !sym->isFunction)
            throw std::runtime_error("Call to undeclared function: " + call->funcName);
        if (sym->params.size() != call->args.size())
            throw std::runtime_error("Wrong number of arguments for " + call->funcName);
        for (size_t i = 0; i < call->args.size(); ++i) {
            Type argType = getExprType(call->args[i]);
            if (!typeCompatible(sym->params[i].type, argType))
                throw std::runtime_error("Argument type mismatch in call to " + call->funcName);
        }
        return sym->type;
    } else if (ArrayAccessExpr* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        // arr->array должен быть IdentifierExpr
        IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(arr->array);
        if (!id) throw std::runtime_error("Array access requires identifier");
        Symbol* sym = current->lookup(id->name);
        if (!sym) throw std::runtime_error("Undeclared array: " + id->name);
        if (!sym->type.isArray) throw std::runtime_error("Cannot index non-array: " + id->name);
        Type idxType = getExprType(arr->index);
        if (!isInteger(idxType)) throw std::runtime_error("Array index must be integer");
        Type elem = sym->type;
        elem.isArray = false;
        elem.arraySize = 0;
        return elem;
    } else if (CommaExpr* comm = dynamic_cast<CommaExpr*>(node)) {
        Type last;
        for (size_t i = 0; i < comm->exprs.size(); ++i) {
            last = getExprType(comm->exprs[i]);
        }
        return last;
    }
    throw std::runtime_error("Unknown expression type");
}

bool SemanticAnalyzer::typeCompatible(const Type& left, const Type& right) {
    if (left.base == right.base && left.isArray == right.isArray) {
        if (left.isArray && left.arraySize != right.arraySize) return false;
        return true;
    }
    // Неявные преобразования между скалярными
    if (!left.isArray && !right.isArray) {
        if ((left.base == TYPE_INT && right.base == TYPE_FLOAT) ||
            (left.base == TYPE_FLOAT && right.base == TYPE_INT))
            return true;
        if ((left.base == TYPE_CHAR && right.base == TYPE_INT) ||
            (left.base == TYPE_INT && right.base == TYPE_CHAR))
            return true;
        if ((left.base == TYPE_BOOL && right.base == TYPE_INT) ||
            (left.base == TYPE_INT && right.base == TYPE_BOOL))
            return true;
    }
    return false;
}

bool SemanticAnalyzer::isScalar(const Type& t) {
    return !t.isArray && t.base != TYPE_VOID;
}

bool SemanticAnalyzer::isArithmetic(const Type& t) {
    return !t.isArray && (t.base == TYPE_INT || t.base == TYPE_FLOAT || t.base == TYPE_CHAR);
}

bool SemanticAnalyzer::isInteger(const Type& t) {
    return !t.isArray && (t.base == TYPE_INT || t.base == TYPE_CHAR || t.base == TYPE_BOOL);
}