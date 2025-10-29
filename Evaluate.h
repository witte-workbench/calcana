#ifndef EVAL_H
#define EVAL_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <regex>

// Forward declaration to avoid including MyTable.h here
class MyTable;
class Cell;

// AST node definition
struct Node {
    std::string type;
    std::string value;
    std::vector<Node*> children;
    Node(const std::string &t, const std::string &v, const std::vector<Node*>& c)
        : type(t), value(v), children(c) {}
    Node(const std::string &t, const std::string &v)
        : type(t), value(v), children({}) {}
};
std::vector<Cell*> get_refs(std::string expression, MyTable* table);
std::vector<std::string> tokenizer(std::string expression);
Node* AST(std::vector<std::string> tokens);
void walkTree(Node* ast, MyTable* table);
std::string evaluateExpression(std::string ex, MyTable* table, std::string* type);

#endif // EVAL_H