#include "Evaluate.h"
#include "MyTable.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <regex>
#include <format>
#include <iomanip>
#include <algorithm>    // std::sort

using namespace std;

string to_lower(string s) {        
    for(char &c : s)
        c = tolower(c);
    return s;
}
string to_upper(string s) {        
    for(char &c : s)
        c = toupper(c);
    return s;
}

std::string toString(double x, int precision = 6) {
    std::ostringstream oss;
    // use fixed so you get a decimal point, then trim zeros
    oss << std::fixed << std::setprecision(precision) << x;
    std::string s = oss.str();

    // strip trailing zeros
    auto pos = s.find('.');
    if (pos != std::string::npos) {
        // erase zeros after decimal
        s.erase(s.find_last_not_of('0') + 1);
        // if the dot is now last, erase it too
        if (s.back() == '.') 
            s.pop_back();
    }
    return s;
}

vector<Cell*> get_refs(string expression, MyTable* table) {
    vector<string> tokens;
    string buffer = "";
    bool string = false;

    for (char ch : expression) {
        if (string) {
            if (ch == '"') string = false;
            continue;
        }
        if (ch == '"') {
            string = true;
            continue;
        }
        if (ch == '(') {
          buffer = "";
          continue;
        }
        if (std::string("!*/+-)%,&=<>|^").find(ch) != std::string::npos) {
        	if (std::regex_match(buffer, std::regex("^[A-Z]+[0-9]+$"))) {
                if (buffer.size() > 0) {
                    tokens.push_back(buffer);
                    buffer = "";
                }
            } else {
                buffer = "";
            }
            continue;
        }
        if (std::regex_match(std::string(1, ch), std::regex("[0-9A-Z]"))) { // number or letter
            buffer += ch;
            continue;
        } else {
            buffer = "";
            continue;
        }
    }
    if (std::regex_match(buffer, std::regex("^[A-Z]+[0-9]+$"))) {
        tokens.push_back(buffer);
        buffer = "";
    }
    vector<Cell*> cells;
    for (std::string cellName : tokens) {
        cells.push_back(table->get_cell(cellName));
    }
    return cells;
}

vector<string> tokenizer(string expression) {
    vector<string> tokens;
    string buffer = "";
    bool string = false;

    for (char ch : expression) {
        if (string) {
            buffer += ch;
            if (ch == '"') string = false;
        } else {
            if (std::string(" \n\t\r").find(ch) != std::string::npos) continue;
            if (ch == '"') {
                string = true;
            }
            if (std::regex_match(std::string(1, ch), std::regex("[0-9a-z.\"]", std::regex::icase))) { // number or letter
                buffer += ch;
                continue;
            }
            if (std::string("!*/+-()%,&=<>|^").find(ch) != std::string::npos) {
                if (buffer.size() > 0) {
                    tokens.push_back(buffer);
                    buffer = "";
                }
                tokens.push_back(std::string(1, ch));
                continue;
            }
            throw ("Tok not found: " + std::string(1,ch));
        }
    }
    if (buffer.size() > 0) {
        tokens.push_back(buffer);
        buffer = "";
    }
    return tokens;
}


Node* AST(vector<string> tokens) {
    if (tokens.size()==0)
        throw std::runtime_error("No tokens found.");
    struct OperatorInfo {
        int lbp; // Left binding power
        int rbp; // Right binding power
    };
    unordered_map<string, OperatorInfo> operator_table = {
        { "+", {10, 10} },
        { "-", {10, 10} },
        { "*", {20, 20} },
        { "/", {20, 20} },
        { "^", {30, 29} }
    };
    vector<string> functions = {"SUM", "AVERAGE","CONCAT","COUNTA","IF","MIN","MAX","LOWER","UPPER","POW"};
    int i = -1;

    auto nextTok = [&] () {
        i++;
        if (i==(int) tokens.size())
            return string("");
        return tokens[i];
    };

    auto peekTok = [&] () {
        if (i+1==(int) tokens.size())
            return string("");
        return tokens[i + 1];
    };

    auto trashTok = [&] () {
        i++;
    };

    function<Node*(int)> parse_expression = [&] (int min_bp) {
        string token = nextTok();
        Node* lhs;
        // Parse prefix
        if (std::regex_match(token, std::regex("^[0-9]+$"))) {
            lhs = new Node("Integer", token);
        } else if (std::regex_match(token, std::regex("^[0-9]+\\.[0-9]+$"))) {
            lhs = new Node("Decimal", token);
        } else if (std::regex_match(token, std::regex("^[0-9]+e[0-9]+$"))) {
            lhs = new Node("Scientific", token);
        } else if (std::regex_match(token, std::regex("^[\\$][0-9]+e[0-9]+$"))) {
            lhs = new Node("Currency", token);
        } else if (std::regex_match(token, std::regex("^[0-9]+e[0-9]+$"))) {
            lhs = new Node("Date", token);
        } else if (token=="TRUE" || token=="FALSE") {
            lhs = new Node("True/False", token);
        } else if (std::find(functions.begin(), functions.end(), token) != functions.end() && peekTok() == "(") {
            string funcName = token;
            vector<Node*> params = {};
            string tok;
            do {
                trashTok();
                params.push_back(parse_expression(0));
            } while (peekTok() == ",");
            if (nextTok() != ")")
                throw ("Function not closed, expected )");
            lhs = new Node("Function", funcName, params);
        } else if (token == "(") {
            lhs = parse_expression(0);
            if (nextTok() != ")") {
                throw std::runtime_error("Expected closing parenthesis");
            }
        } else if (token == ")") {
            throw ("Unexpected extra )");
        } else if (token[0]=='"') {
            lhs = new Node("Text", token.substr(1,token.size()-2));
        } else {
            lhs = new Node("Identifier", token);
        }

        // Parse infix operators
        while (std::string("*/+-%&=<>|^").find(peekTok()[0]) != std::string::npos && operator_table[peekTok()].lbp >= min_bp) {
            string op = nextTok();
            OperatorInfo bp = operator_table[op];
            Node* rhs = parse_expression(bp.rbp);
            lhs = new Node("Operator", op, {lhs, rhs});
        }
        return lhs;
    };
    Node* result = parse_expression(0);
    if (peekTok() != "")
        throw std::runtime_error("Unexpected token: "+peekTok());
    return result;
}

void walkTree(Node* ast, MyTable* table) {
    for (Node* child : ast->children) {
        if (child->type == "Identifier" || child->type == "Function" || child->type == "Operator") {
            walkTree(child, table);
        }
    }
    if (ast->type == "Identifier") {
        ast->type = table->get_cell(ast->value)->computed_type;
        ast->value = table->get_value(ast->value);
        ast->children = {};
    } else if (ast->type == "Function") {
        if (ast->value == "SUM") {
            float acc = 0;
            for (Node* child : ast->children) {
                acc += stof(child->value);
            }
            ast->value = toString(acc);
            ast->type = "Decimal";
        } else if (ast->value == "AVERAGE") {
            float acc = 0;
            for (Node* child : ast->children) {
                acc += stof(child->value);
            }
            ast->value = toString(acc / ast->children.size());
            ast->type = "Decimal";
        } else if (ast->value == "CONCAT") {
            string acc = "";
            for (Node* child : ast->children) {
                acc += child->value;
            }
            ast->value = acc;
            ast->type = "Text";
        } else if (ast->value == "COUNTA") {
            ast->value = toString(ast->children.size());
            ast->type = "Integer";
        } else if (ast->value == "COUNTBLANK") {
            float acc = 0;
            for (Node* child : ast->children) {
                if (child->value.size()==0)
                    acc++;
            }
            ast->value = toString(acc);
            ast->type = "Integer";
        } else if (ast->value == "IF") {
            ast->value = ast->children[0]->value=="TRUE" ? ast->children[1]->value : ast->children[2]->value;
            ast->type = ast->children[0]->value=="TRUE" ? ast->children[1]->type : ast->children[2]->type;
        } else if (ast->value == "MAX") {
            float acc = stof(ast->children[0]->value);
            for (Node* child : ast->children) {
                if (acc < stof(child->value))
                    acc = stof(child->value);
            }
            ast->value = toString(acc);
            ast->type = "Decimal";
        } else if (ast->value == "MIN") {
            float acc = stof(ast->children[0]->value);
            for (Node* child : ast->children) {
                if (acc > stof(child->value))
                    acc = stof(child->value);
            }
            ast->value = toString(acc);
            ast->type = "Decimal";
        } else if (ast->value == "LOWER") {
            ast->value = to_lower(ast->children[0]->value);
            ast->type = "Text";
        } else if (ast->value == "UPPER") {
            ast->value = to_upper(ast->children[0]->value);
            ast->type = "Text";
        } else if (ast->value == "POW") {
            float base = stof(ast->children[0]->value);
            int exponent = stoi(ast->children[1]->value);
            float acc = base;
            for (int i=1; i<exponent; i++) {
                acc *= base;
            }
            ast->value = toString(acc);
            ast->type = "Decimal";
        }
    } else if (ast->type == "Operator") {
        if (ast->value == "+") {
            ast->value = toString(stoi(ast->children[0]->value) + stoi(ast->children[1]->value));
        } else if (ast->value == "-") {
            ast->value = toString(stoi(ast->children[0]->value) - stoi(ast->children[1]->value));
        } else if (ast->value == "*") {
            ast->value = toString(stoi(ast->children[0]->value) * stoi(ast->children[1]->value));
        } else if (ast->value == "/") {
            ast->value = toString(stoi(ast->children[0]->value) / stoi(ast->children[1]->value));
        } else if (ast->value == "^") {
            ast->value = toString(pow (stoi(ast->children[0]->value) , stoi(ast->children[1]->value )));
        }
        ast->type = "Decimal";
        ast->children = {};
    }
}


string evaluateExpression(string ex, MyTable* table, std::string* type) {
    vector<string> tokens = tokenizer(ex);
    Node* ast = AST(tokens);
    walkTree(ast, table);
    *type = ast->type;
    return ast->value;
}