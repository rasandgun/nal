#include "codegen.h"
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <algorithm>


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

void CodeGenerator::emitAssignment(Expression* left, const std::string& op, Expression* right) {
    if (auto* id = dynamic_cast<IdentifierExpr*>(left)) {
        if (op != "=") {
            commands.push_back(Command(LOAD, id->name));   
            visitNode(right);                              
            std::string baseOp = op.substr(0, op.size() - 1);
            commands.push_back(Command(binaryOpToInst(baseOp)));
        } else {
            visitNode(right);
        }
        
        commands.push_back(Command(DUP));
        commands.push_back(Command(ASSIGN, id->name));
    }
    else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(left)) {
        visitNode(arr->index);
        if (op != "=") {
            commands.push_back(Command(DUP));
            commands.push_back(Command(LOAD_ARRAY, arr->array->name));
            visitNode(right);
            std::string baseOp = op.substr(0, op.size() - 1);
            commands.push_back(Command(binaryOpToInst(baseOp)));
        } else {
            visitNode(right);
        }
        
        commands.push_back(Command(SWAP));  
        commands.push_back(Command(OVER));  
        commands.push_back(Command(STORE_ARRAY, arr->array->name));
    }
    else {
        throw std::runtime_error("Invalid left side of assignment");
    }
}

void CodeGenerator::visitNode(ASTNode* node) {
    if (!node) return;
    if (auto* program = dynamic_cast<Program*>(node)) {
        for (auto decl : program->declarations) {
            visitNode(decl);
        }
    }
    else if (auto* func = dynamic_cast<FunctionDecl*>(node)) {
        functionStart[func->name] = commands.size();
        commands.push_back(Command(SCOPEOPEN));
        for (const auto& p : func->params) {
            commands.push_back(Command(DECLAREVAR, p.type, p.name));
            commands.push_back(Command(ASSIGN, p.name));
        }
        openedScopes = 1;
        visitNode(func->body);
        if (commands.empty() || commands.back().inst != RET) {
            commands.push_back(SCOPEPOP);
            commands.push_back(Command(RET));
        }
    }
    else if (auto* block = dynamic_cast<BlockStmt*>(node)) {
        openedScopes++;
        commands.push_back(Command(SCOPEOPEN));
        for (auto stmt : block->statements) {
            visitNode(stmt);
        }
        openedScopes--;
        commands.push_back(Command(SCOPEPOP));
    }
    else if (auto* ifs = dynamic_cast<IfStmt*>(node)) {
        visitNode(ifs->condition);
        size_t jumpToElse = emitPlaceholder(JUMPIFFALSE);
        visitNode(ifs->thenBranch);
        size_t jumpToEnd = 0;
        if (ifs->elseBranch) {
            jumpToEnd = emitPlaceholder(JUMP);
        }
        commands[jumpToElse].argVal.i32 = (int)commands.size();
        if (ifs->elseBranch) {
            visitNode(ifs->elseBranch);
            commands[jumpToEnd] = Command(JUMP, (int)commands.size());
        }
    }
    else if (auto* wh = dynamic_cast<WhileStmt*>(node)) {
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t start = currentInstruction();
        visitNode(wh->condition);
        commands.push_back(Command(NOT));
        size_t jumpOut = emitPlaceholder(JUMPIFTRUE);
        visitNode(wh->body);
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)start);
        commands.push_back(Command(JUMP, (int)start));
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        commands[jumpOut].argVal.i32 = (int)afterLoop;
        loopStack.pop_back();
    }
    else if (auto* dw = dynamic_cast<DoWhileStmt*>(node)) {
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t bodyStart = currentInstruction();
        visitNode(dw->body);
        size_t condStart = currentInstruction();
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)condStart);
        visitNode(dw->condition);
        commands.push_back(Command(JUMPIFTRUE, (int)bodyStart));
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        loopStack.pop_back();
    }
    else if (auto* fr = dynamic_cast<ForStmt*>(node)) {
        commands.push_back(Command(SCOPEOPEN));
        if (fr->init) visitNode(fr->init);
        loopStack.push_back(LoopInfo{currentInstruction(), {}, {}});
        size_t condStart = currentInstruction();
        if (fr->condition) {
            visitNode(fr->condition);
        } else {
            commands.push_back(Command(PUSH, 1));
        }
        commands.push_back(Command(NOT));
        size_t jumpOut = emitPlaceholder(JUMPIFTRUE);
        visitNode(fr->body);
        size_t updateStart = currentInstruction();
        for (auto addr : loopStack.back().continueAddr)
            commands[addr] = Command(JUMP, (int)updateStart);
        if (fr->update) {
            visitNode(fr->update);
            if (fr->update->type.base != TYPE_VOID) {
                commands.push_back(Command(POP));
            }
        }
        commands.push_back(Command(JUMP, (int)condStart));
        size_t afterLoop = currentInstruction();
        for (auto addr : loopStack.back().breakAddr)
            commands[addr] = Command(JUMP, (int)afterLoop);
        commands[jumpOut].argVal.i32 = (int)afterLoop;
        commands.push_back(Command(SCOPEPOP));
        loopStack.pop_back();
    }
    else if (auto* ret = dynamic_cast<ReturnStmt*>(node)) {
        if (ret->expr) {
            visitNode(ret->expr);
        }
        
        for (size_t i = 0; i < openedScopes; i++)
            commands.push_back(Command(SCOPEPOP));
        commands.push_back(Command(RET));
    }
    else if (dynamic_cast<BreakStmt*>(node)) {
        if (loopStack.empty()) throw std::runtime_error("break outside loop");
        loopStack.back().breakAddr.push_back(currentInstruction());
        commands.push_back(Command());
    }
    else if (dynamic_cast<ContinueStmt*>(node)) {
        if (loopStack.empty()) throw std::runtime_error("continue outside loop");
        loopStack.back().continueAddr.push_back(currentInstruction());
        commands.push_back(Command());
    }
    else if (auto* es = dynamic_cast<ExprStmt*>(node)) {
        visitNode(es->expr);
        
        if (es->expr->type.base != TYPE_VOID) {
            commands.push_back(Command(POP));
        }
    }
    else if (auto* var = dynamic_cast<VarDecl*>(node)) {
        commands.push_back(Command(DECLAREVAR, var->type, var->name));
        if (var->init) {
            if (var->type.isArray && var->type.base == TYPE_CHAR) {
                auto* lit = dynamic_cast<LiteralExpr*>(var->init);
                if (lit && lit->kind == LiteralExpr::LIT_STRING) {
                    commands.push_back(Command(INIT_ARRAY_STRING, var->name, lit->value));
                    return;
                }
            }
            visitNode(var->init);
            commands.push_back(Command(ASSIGN, var->name));
        }
    }
    else if (auto* lit = dynamic_cast<LiteralExpr*>(node)) {
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
    else if (auto* id = dynamic_cast<IdentifierExpr*>(node)) {
        commands.push_back(Command(LOAD, id->name));
    }
    else if (auto* arr = dynamic_cast<ArrayAccessExpr*>(node)) {
        visitNode(arr->index);
        commands.push_back(Command(LOAD_ARRAY, arr->array->name));
    }
    else if (auto* un = dynamic_cast<UnaryExpr*>(node)) {
        visitNode(un->operand);
        if (un->op == "-") {
            commands.push_back(Command(NEG));
        } else if (un->op == "!") {
            commands.push_back(Command(NOT));
        } else if (un->op == "+") {
            
        } else {
            throw std::runtime_error("Unknown unary operator: " + un->op);
        }
    }
    else if (auto* bin = dynamic_cast<BinaryExpr*>(node)) {
        if (bin->op == "=" || bin->op == "+=" || bin->op == "-=" ||
            bin->op == "*=" || bin->op == "/=" || bin->op == "%=") {
            emitAssignment(bin->left, bin->op, bin->right);
        } else {
            visitNode(bin->left);
            visitNode(bin->right);
            commands.push_back(Command(binaryOpToInst(bin->op)));
        }
    }
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
    else if (auto* comma = dynamic_cast<CommaExpr*>(node)) {
        for (size_t i = 0; i < comma->exprs.size(); ++i) {
            visitNode(comma->exprs[i]);
            if (i != comma->exprs.size() - 1) {
                if (comma->exprs[i]->type.base != TYPE_VOID) {
                    commands.push_back(Command(POP));
                }
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

    commands.push_back(Command(SCOPEOPEN));
    for (auto var : globals)
        commands.push_back(Command(DECLAREVAR, var->type, var->name));

    for (auto var : globals) {
        if (var->init) {
            visitNode(var->init);
            commands.push_back(Command(ASSIGN, var->name));
        }
    }

    commands.push_back(Command(CALL, "main"));
    commands.push_back(Command(POP));   
    commands.push_back(Command(HALT));

    for (auto func : functions) {
        visitNode(func);
    }
}