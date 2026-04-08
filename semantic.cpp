#include "semantic.h"
#include <stdexcept>
#include <map>

SemanticAnalyzer::Scope::Scope(Scope* p) : parent(p) {}

SemanticAnalyzer::Scope::~Scope() {
}

void SemanticAnalyzer::Scope::declare(const std::string& name, const Type& type, bool isFunc, const std::vector<Parameter>& params) {
    if (symbols.find(name) != symbols.end())
        throw std::runtime_error("Symbol already declared in this scope: " + name);
    Symbol sym;
    sym.type = type;
    sym.isFunction = isFunc;
    sym.params = params;
    symbols[name] = sym;
}

void SemanticAnalyzer::Scope::define(const std::string& name) {
    Symbol* s = lookup(name);
    if (!s) throw std::runtime_error("Symbol not found: " + name);
}

SemanticAnalyzer::Symbol* SemanticAnalyzer::Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) return &it->second;
    if (parent) return parent->lookup(name);
    return nullptr;
}


Type* SemanticAnalyzer::Scope::getType(const std::string& name) {
    Symbol* s = lookup(name);
    if (s) return &s->type;
    return nullptr;
}

bool SemanticAnalyzer::Scope::isFunction(const std::string& name) {
    Symbol* s = lookup(name);
    return s && s->isFunction;
}

std::vector<Parameter>* SemanticAnalyzer::Scope::getParams(const std::string& name) {
    Symbol* s = lookup(name);
    if (s && s->isFunction) return &s->params;
    return nullptr;
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

SemanticAnalyzer::SemanticAnalyzer() {
    global = new Scope();
    current = global;
    inLoop = false;
}

SemanticAnalyzer::~SemanticAnalyzer() {
    delete global;
}

void SemanticAnalyzer::analyze(Program* program) {
    collectDeclarations(program);
    analyzeNode(program);
}

void SemanticAnalyzer::collectDeclarations(Program* prog) {
    for (size_t i = 0; i < prog->declarations.size(); ++i) {
        ASTNode* node = prog->declarations[i];
        if (FunctionDecl* func = dynamic_cast<FunctionDecl*>(node)) {
            global->declare(func->name, func->returnType, true, func->params);
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
        foundReturn = false;
        current = current->pushScope();
        for (size_t i = 0; i < func->params.size(); ++i) {
            Parameter& p = func->params[i];
            current->declare(p.name, p.type);
            current->define(p.name);
        }
        Type prevRet = currentReturnType;
        currentReturnType = func->returnType;
        bool prevLoop = inLoop;
        inLoop = false;
        analyzeNode(func->body);
        if (func->returnType != Type(TYPE_VOID) && !foundReturn) {
            throw std::runtime_error("Couldn't find return statement in function " + func->name);
        }
        currentReturnType = prevRet;
        inLoop = prevLoop;
        current = current->popScope();
    } else if (VarDecl* var = dynamic_cast<VarDecl*>(node)) {
        if (current->lookup(var->name) != nullptr) {
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
        Scope *prev = current;
        current = current->pushScope();
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
        foundReturn = true;
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
        getExprType(bin);
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
        // do nothing
    } else {
        // unknown node
        throw std::runtime_error("Unknown node type");
    }
}

static std::string getLValueName(Expression* expr) {
    if (IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(expr)) {
        return id->name;
    } else if (ArrayAccessExpr* arr = dynamic_cast<ArrayAccessExpr*>(expr)) {
        return arr->array->name;
    } else {
        throw std::runtime_error("Expression is not a valid lvalue");
    }
}

Type SemanticAnalyzer::getExprType(Expression* node) {
    if (LiteralExpr* lit = dynamic_cast<LiteralExpr*>(node)) {
        Type t;
        switch (lit->kind) {
            case LiteralExpr::LIT_INT:    t = Type(TYPE_INT); break;
            case LiteralExpr::LIT_FLOAT:  t = Type(TYPE_FLOAT); break;
            case LiteralExpr::LIT_CHAR:   t = Type(TYPE_CHAR); break;
            case LiteralExpr::LIT_STRING: t = Type(TYPE_CHAR, true, lit->value.size()); break;
            case LiteralExpr::LIT_BOOL:   t = Type(TYPE_BOOL); break;
        }
        node->type = t;
        return t;
    } 
    else if (IdentifierExpr* id = dynamic_cast<IdentifierExpr*>(node)) {
        Symbol* sym = current->lookup(id->name);
        if (!sym) throw std::runtime_error("Undeclared identifier: " + id->name);
        node->type = sym->type;
        return sym->type;
    }
    else if (BinaryExpr* bin = dynamic_cast<BinaryExpr*>(node)) {
        Type left = getExprType(bin->left);
        Type right = getExprType(bin->right);
        std::string op = bin->op;
        Type result;

        if (op == "=" || op == "*=" || op == "/=" || op == "%=" || op == "+=" || op == "-=") {
            if (!dynamic_cast<IdentifierExpr*>(bin->left) && !dynamic_cast<ArrayAccessExpr*>(bin->left))
                throw std::runtime_error("Left side of assignment must be lvalue");
            if (!typeCompatible(left, right))
                throw std::runtime_error("Type mismatch in assignment");
            result = left;
            std::string varName = getLValueName(bin->left);
            Symbol* sym = current->lookup(varName);
            if (!sym) throw std::runtime_error("Symbol not found: " + varName);
        }
        else if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (!isArithmetic(left) || !isArithmetic(right))
                throw std::runtime_error("Arithmetic operation requires arithmetic types");
            result = (left.base == TYPE_FLOAT || right.base == TYPE_FLOAT) ? Type(TYPE_FLOAT) : Type(TYPE_INT);
        }
        else if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            if (!typeCompatible(left, right))
                throw std::runtime_error("Comparison of incompatible types");
            result = Type(TYPE_BOOL);
        }
        else if (op == "&&" || op == "||") {
            if (!isScalar(left) || !isScalar(right))
                throw std::runtime_error("Logical operation requires scalar types");
            result = Type(TYPE_BOOL);
        }
        else {
            throw std::runtime_error("Unknown operator: " + op);
        }
        node->type = result;
        return result;
    }
    else if (UnaryExpr* un = dynamic_cast<UnaryExpr*>(node)) {
        Type opnd = getExprType(un->operand);
        Type result;
        if (un->op == "+" || un->op == "-") {
            if (!isArithmetic(opnd))
                throw std::runtime_error("Unary +/- requires arithmetic type");
            result = opnd;
        }
        else if (un->op == "!") {
            if (!isScalar(opnd))
                throw std::runtime_error("Unary ! requires scalar type");
            result = Type(TYPE_BOOL);
        }
        else {
            throw std::runtime_error("Unknown unary operator");
        }
        node->type = result;
        return result;
    }
    else if (CallExpr* call = dynamic_cast<CallExpr*>(node)) {
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
        node->type = sym->type;
        return sym->type;
    }
    else if (ArrayAccessExpr* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        IdentifierExpr* id = arr->array;
        Symbol* sym = current->lookup(id->name);
        if (!sym) throw std::runtime_error("Undeclared array: " + id->name);
        if (!sym->type.isArray) throw std::runtime_error("Cannot index non-array: " + id->name);
        Type idxType = getExprType(arr->index);
        if (!isInteger(idxType)) throw std::runtime_error("Array index must be integer");
        Type elem = sym->type;
        elem.isArray = false;
        elem.size = 0;
        node->type = elem;
        return elem;
    }
    else if (CommaExpr* comm = dynamic_cast<CommaExpr*>(node)) {
        Type last;
        for (size_t i = 0; i < comm->exprs.size(); ++i) {
            last = getExprType(comm->exprs[i]);
        }
        node->type = last;
        return last;
    }
    throw std::runtime_error("Unknown expression type");
}

bool SemanticAnalyzer::typeCompatible(const Type& left, const Type& right) {
    if (left.isArray && right.isArray) {
        if (left.base != right.base) return false;
        if (left.size != right.size) return false;
        return true;
    }
    if (left.isArray != right.isArray) return false;
    if (left.base == right.base) return true;
    if ((left.base == TYPE_INT && right.base == TYPE_FLOAT) ||
        (left.base == TYPE_FLOAT && right.base == TYPE_INT))
        return true;
    if ((left.base == TYPE_CHAR && right.base == TYPE_INT) ||
        (left.base == TYPE_INT && right.base == TYPE_CHAR))
        return true;
    if ((left.base == TYPE_BOOL && right.base == TYPE_INT) ||
        (left.base == TYPE_INT && right.base == TYPE_BOOL))
        return true;
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