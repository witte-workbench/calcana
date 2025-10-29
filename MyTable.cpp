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

#include "MyTable.h"
#include "Cell.h"



MyTable::MyTable(int X, int Y, int W, int H, int nRows, int nCols)
: Fl_Table_Row(X, Y, W, H), data(nRows, vector<Cell>(nCols))
{
    rows(nRows);
    cols(nCols);
    row_header(1);
    col_header(1);
    // Pre-calculate all cell values
    for (int r = 0; r < nRows; ++r) {
        for (int c = 0; c < nCols; ++c) {
            string v = generate_cell_value();
            data[r][c] = Cell(r, c, v, this);
        }
    }
    end();
}

void MyTable::save_file() {
    ofstream MyFile("sheet.cna");
    for (const auto& row : data) {
        for (const auto& cell : row) {
            MyFile << cell.raw_value << "|";
        }
        MyFile << "\n";
    }
}

void MyTable::load_file() {
    ifstream MyFile("sheet.cna");
    string line;
    int r = 0;
    while (getline(MyFile, line) && r < (int)data.size()) {
        stringstream ss(line);
        string token;
        int c = 0;
        while (getline(ss, token, '|') && c < (int)data[r].size()) {
            data[r][c].set_raw(token, false);
            ++c;
        }
        ++r;
    }
    redraw();
}

Cell* MyTable::get_cell(string cellName) {
    int col = 0;
    int row = 0;
    size_t i = 0;
    // Parse column (letters)
    while (i < cellName.size() && std::isalpha(cellName[i])) {
        col = col * 26 + (std::toupper(cellName[i]) - 'A' + 1);
        ++i;
    }
    // Parse row (digits)
    while (i < cellName.size() && std::isdigit(cellName[i])) {
        row = row * 10 + (cellName[i] - '0');
        ++i;
    }
    return &data[row-1][col-1];
}

string MyTable::get_value(string cellName) {
    return get_cell(cellName)->computed_value;
}

string MyTable::generate_cell_value() {
    return "";//to_string(rand() % 101);
}
string MyTable::col_label(int c) {
    string s;
    for (int x = c; x >= 0; x = x/26 - 1)
        s.insert(s.begin(), char('A' + x%26));
    return s;
}
void MyTable::draw_cell(TableContext context, int R, int C,
            int X, int Y, int W, int H)
{
    switch (context) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, 12);
            return;
        case CONTEXT_COL_HEADER:
            fl_push_clip(X,Y,W,H);
            fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, color());
            fl_color(FL_BLACK);
            fl_draw(col_label(C).c_str(),
                    X + W/2 - fl_width("W")/2, Y + H/2 + 4);
            fl_pop_clip();
            return;
        case CONTEXT_ROW_HEADER:
            fl_push_clip(X,Y,W,H);
            fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, color());
            fl_color(FL_BLACK);
            {
                string num = to_string(R+1);
                fl_draw(num.c_str(),
                        X + W/2 - fl_width(num.c_str())/2,
                        Y + H/2 + 4);
            }
            fl_pop_clip();
            return;
        case CONTEXT_CELL:
            fl_push_clip(X,Y,W,H);
            fl_draw_box(FL_THIN_DOWN_BOX, X,Y,W,H, is_selected(R,C)
                      ? fl_rgb_color(255,255,180)   // highlight
                      : FL_WHITE);
            fl_color(FL_BLACK);
            fl_draw(data[R][C].computed_value.c_str(),
                    X + 4, Y + H/2 + 4);
            fl_pop_clip();
            return;
        default:
            return;
    }
}
int MyTable::handle(int event) {
    int base = Fl_Table_Row::handle(event);
    if (callback_context() == CONTEXT_CELL) {
        if (event == FL_PUSH
            && Fl::event_button() == FL_LEFT_MOUSE
            && Fl::event_clicks() == 1)
        {
            int R = callback_row(), C = callback_col();
            const char* newval = fl_input("Edit cell:", data[R][C].raw_value.c_str());
            if (newval) {
                data[R][C].set_raw(newval, false);
                redraw();
            }
            return 1;
        } else if (event == FL_PUSH
            && Fl::event_button() == FL_LEFT_MOUSE
            && callback_context() == CONTEXT_CELL) {
            int r, c;
            Fl_Table::ResizeFlag rf;
            Fl_Table::TableContext ctx = cursor2rowcol(r, c, rf);

            if (ctx == Fl_Table::CONTEXT_CELL) {
                selectionMade = true;
                selStartR = r;
                selStartC = c;
                selEndR = r;
                selEndC = c;
            }
            redraw();
            return 1;
        } else if (event == FL_DRAG) {
            int r, c;
            Fl_Table::ResizeFlag rf;
            Fl_Table::TableContext ctx = cursor2rowcol(r, c, rf);

            if (ctx == Fl_Table::CONTEXT_CELL) {
                selEndR = r;
                selEndC = c;
            }
            redraw();
            return 1;
        } else if (event == FL_RELEASE) {
            int r, c;
            Fl_Table::ResizeFlag rf;
            Fl_Table::TableContext ctx = cursor2rowcol(r, c, rf);

            if (ctx == Fl_Table::CONTEXT_CELL) {
                selEndR = r;
                selEndC = c;
                string commonType = data[r][c].type;
                string commonValue = data[r][c].raw_value;
                for (int i=min(selStartR,selEndR); i<=max(selStartR,selEndR); i++) {
                    for (int j=min(selStartC,selEndC); j<=max(selStartC,selEndC); j++) {
                        if (data[i][j].raw_value != commonValue) {
                            commonValue = "?";
                            goto return_common_values;
                        }
                        if (data[i][j].type != commonType) {
                            commonType = "?";
                            goto return_common_values;
                        }
                    }
                }
                return_common_values:
                    selectionUpdateFunc(commonType, commonValue);
            }
            redraw();
            return 1;
        }
    }
    if (event == FL_KEYDOWN) {
        int keyId = Fl::event_key();
        if (keyId == FL_Escape) {
            selectionMade = false;
        } else if (keyId==65362) { // up
            selStartR--;
            selEndR--;
        } else if (keyId==65363) { // right
            selStartC++;
            selEndC++;
        } else if (keyId==65364) { // down
            selStartR++;
            selEndR++;
        } else if (keyId==65361) { // left
            selStartC--;
            selEndC--;
        }
        redraw();
    }
    return base;
}

void MyTable::setSelectedType(string type) {
    if (selStartR<0 || selEndR<0 || selStartC<0 || selEndC<0)
        return;
    for (int i=min(selStartR,selEndR); i<=max(selStartR,selEndR); i++) {
        for (int j=min(selStartC,selEndC); j<=max(selStartC,selEndC); j++) {
            data[i][j].type = type;
        }
    }
}
void MyTable::setSelectedValue(string value) {
    if (selStartR<0 || selEndR<0 || selStartC<0 || selEndC<0)
        return;
    for (int i=min(selStartR,selEndR); i<=max(selStartR,selEndR); i++) {
        for (int j=min(selStartC,selEndC); j<=max(selStartC,selEndC); j++) {
            data[i][j].set_raw(value, true);
        }
    }
}

void MyTable::do_redraw() {
    redraw();
}