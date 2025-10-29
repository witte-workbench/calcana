#ifndef CELL_H
#define CELL_H

#include <vector>
#include <string>

// Forward declarations to break circular includes
class MyTable;
std::string evaluateExpression(std::string ex, MyTable* table);

class Cell {
public:
    int row;
    int col;
    std::string raw_value;
    std::string computed_value;
    std::string type;
    std::string computed_type;
    bool err;
    MyTable* table;
    std::vector<Cell*> referenced_by;
    std::vector<Cell*> references_to;

    Cell()
        : row(0), col(0), raw_value(""), computed_value(""), table(nullptr) {}
    Cell(int r, int c, const std::string& v, MyTable* t);

    void set_raw(std::string value, bool doRedraw);

    void compute_value();

    void compute_type();
};

#endif // CELL_H