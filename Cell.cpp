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
#include <algorithm>    // std::find
using namespace std;

#include "MyTable.h"
#include "Cell.h"
#include "Evaluate.h"



Cell::Cell(int r, int c, const string& v, MyTable* t) {
    row = r;
    col = c;
    type = "Auto";
    err = false;
    set_raw(v, false);
    table = t;
    referenced_by = {};
}

void Cell::set_raw(string value, bool doRedraw) {
    if (value.size()==0 || value.at(0) != '=') {
        for (Cell* ref : references_to) { // remove old references
            ref->referenced_by.erase(
                std::remove(ref->referenced_by.begin(), ref->referenced_by.end(), this),
                ref->referenced_by.end()
            );
        }
        references_to = {};
    } else {
        vector<Cell*> refs = get_refs(value.substr(1), table);
        if (std::count(refs.begin(), refs.end(), this) > 0) {
            cout << "Error: Self referencing cell! Undone." << endl;
            return;
        }
        // fix new references
        for (Cell* ref : references_to) { // remove old references
            ref->referenced_by.erase(
                std::remove(ref->referenced_by.begin(), ref->referenced_by.end(), this),
                ref->referenced_by.end()
            );
        }
        for (Cell* ref : refs) {
            ref->referenced_by.push_back(this);
        }
        references_to = refs;
    }
    
    raw_value = value;
    compute_value();
    if (doRedraw)
        table->do_redraw();

}

void Cell::compute_value() {
    if (raw_value.size()==0) {
        computed_value = "";
        return;
    }
    if (raw_value.at(0) != '=') {
        computed_value = raw_value;
        this->compute_type();
    } else {
        try {
            string ret_type;
            computed_value = evaluateExpression(raw_value.substr(1), table, &ret_type);
            this->computed_type = ret_type;
            this->err = false;
        } catch(int errCode) {
            this->err = true;
            this->computed_value = "ERR";
        }
    }

    
    // compute downstream referenced cells
    for (Cell* ref : referenced_by) {
        ref->compute_value();
    }
}

void Cell::compute_type() {
    if (this->type!="auto")
        this->computed_type = this->type;

    // parse types from most to least specific

    // if (this->computed_value.length()==0) {
    //     this->computed_type = "Empty";
    // }
    if (this->computed_value=="TRUE" || this->computed_value=="FALSE") {
        this->computed_type = "True/False";
    } else if (std::regex_match(this->computed_value, std::regex("^[0-9]+$"))) {
        this->computed_type = "Integer";
    } else if (std::regex_match(this->computed_value, std::regex("^[0-9]+\\.[0-9]+$"))) {
        this->computed_type = "Decimal";
    } else if (std::regex_match(this->computed_value, std::regex("^[0-9]\\.?[0-9]?e[0-9]+$"))) {
        this->computed_type = "Scientific";
    } else if (std::regex_match(this->computed_value, std::regex("^\\$[0-9]+\\.?[0-9]?$"))) {
        this->computed_type = "Currency";
    } else if (std::regex_match(this->computed_value, std::regex("^[0-9]{2}\\/[0-9]{2}\\/[0-9]{4}$"))) {
        this->computed_type = "Date";
    } else {
        this->computed_type = "Text";
    }
}