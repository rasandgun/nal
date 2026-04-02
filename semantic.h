#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <map>
class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(Program* program);
private:
    // Внутренние структуры для таблицы символов
    struct Symbol {
        Type type;
        bool isFunction;
        std::vector<Parameter> params;
    };
    class Scope {
    public:
        Scope(Scope* parent = NULL);
        ~Scope();
        void declare(const std::string& name, const Type& type, bool isFunc = false, const std::vector<Parameter>& params = std::vector<Parameter>());
        void define(const std::string& name);
        Symbol* lookup(const std::string& name);
        Type* getType(const std::string& name);
        bool isFunction(const std::string& name);
        std::vector<Parameter>* getParams(const std::string& name);
        Scope* pushScope();
        Scope* popScope();
    private:
        std::map<std::string, Symbol> symbols;
        Scope* parent;
    };

    Scope* global;
    Scope* current;
    Type currentReturnType;
    bool inLoop;

    // Вспомогательные методы
    void collectDeclarations(Program* prog);
    void analyzeNode(ASTNode* node);
    Type getExprType(Expression* node);
    bool typeCompatible(const Type& left, const Type& right);
    bool isScalar(const Type& t);
    bool isArithmetic(const Type& t);
    bool isInteger(const Type& t);
};

#endif