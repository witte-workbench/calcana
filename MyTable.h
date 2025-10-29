#ifndef TABLE_H
#define TABLE_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>       // for fl_input()
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <regex>
using namespace std;

#include "Cell.h"


class MyTable : public Fl_Table_Row {
    vector<vector<Cell>> data;
public:
    MyTable(int X, int Y, int W, int H, int nRows, int nCols);
    ~MyTable() noexcept override = default;

    void save_file();

    void load_file();

    Cell* get_cell(string cellName);
    string get_value(string cellName);

    int selStartR = -1, selStartC = -1, selEndR = -1, selEndC = -1;
    bool selectionMade = false;
    function<void(string, string)> selectionUpdateFunc = nullptr;
    void setSelectedType(string);
    void setSelectedValue(string);
    void do_redraw();

private:
    static string generate_cell_value();
    static string col_label(int c);
    void draw_cell(TableContext context, int R, int C,
                int X, int Y, int W, int H) override;
    int handle(int event) override;

    bool is_selected(int r,int c) const {
        return (selectionMade &&
                r>=std::min(selStartR,selEndR) &&
                r<=std::max(selStartR,selEndR) &&
                c>=std::min(selStartC,selEndC) &&
                c<=std::max(selStartC,selEndC));
    }
};


#endif // TABLE_H