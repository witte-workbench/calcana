#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>       // for fl_input()
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <regex>
using namespace std;

#include "Cell.h"
#include "MyTable.h"








// Toolbar callbacks
static void updateValue(Fl_Widget* w, void* u) {
    MyTable* table = (MyTable*) u;
    auto ch   = static_cast<Fl_Input*>(w);
    auto item = ch->value();
    table->setSelectedValue(item);
    Fl::focus(nullptr);
}
static void btn1_cb(Fl_Widget* w, void* u) {
    MyTable* table = (MyTable*) u;
    table->save_file();
}
static void btn2_cb(Fl_Widget* w, void* u) {
    MyTable* table = (MyTable*) u;
    table->load_file();
}
// Status callback
static void tab1_cb(Fl_Widget* w, void* u) {
    cout << "hi!" << endl;
}
void dropdown_cb(Fl_Widget *w, void* u) {
    MyTable* table = (MyTable*) u;
    auto ch   = static_cast<Fl_Choice*>(w);
    auto item = ch->mvalue();                 // pointer to chosen Fl_Menu_Item
    table->setSelectedType(item->label());
}
int main() {
    const int WIDTH=800, HEIGHT=600;
    const int TOOLBAR_H=60, STATUS_H=20;
    const int W=1000, H=1000;

    Fl_Window win(WIDTH, HEIGHT, "Spreadsheet Demo");

    // Toolbar
    Fl_Button *btn1, *btn2;//, *textConfirm;
    Fl_Choice *dropdown;
    Fl_Input *textInput;
    Fl_Group toolbar(0,0,WIDTH,TOOLBAR_H);
        btn1 = new Fl_Button(5, 5, 80, 20, "Save Data");
        btn2 = new Fl_Button(90,5, 80, 20, "Load Data");
        dropdown = new Fl_Choice(225, 5, 120, 20, "Type");
        dropdown->add("Auto");
        dropdown->add("Text");
        dropdown->add("Integer");
        dropdown->add("Decimal");
        dropdown->add("Sceintific");
        dropdown->add("Date");
        dropdown->add("Currency");
        dropdown->add("True\\/False");
        dropdown->value(0);
        textInput = new Fl_Input(45, 30, 600, 20, "Value");
        // textConfirm = new Fl_Button(650,30, 80, 20, "✓");
    toolbar.end();

    // Table area
    MyTable table(0, TOOLBAR_H, WIDTH, HEIGHT-TOOLBAR_H-STATUS_H, W, H);
    Fl_Group tableArea(0,TOOLBAR_H,WIDTH,HEIGHT-TOOLBAR_H-STATUS_H);
    tableArea.resizable(&table);
    tableArea.end();

    table.selectionUpdateFunc = [dropdown, textInput](string type, string val) {
        dropdown->value( dropdown->find_item(type.c_str()) );
        textInput->value( val.c_str() );
    };

    // Status bar
    Fl_Button *tab1Btn;
    Fl_Group tabs(0, HEIGHT-STATUS_H, WIDTH, STATUS_H);
    tab1Btn = new Fl_Button(5, HEIGHT-STATUS_H+2,
                                WIDTH-10, STATUS_H-4, "Tab 1");
    tabs.end();

    // Assign callbacks
    btn1->callback(btn1_cb, &table);
    btn2->callback(btn2_cb, &table);
    tab1Btn->callback(tab1_cb, &table);
    dropdown->callback(dropdown_cb, &table);
    dropdown->when(FL_WHEN_CHANGED);
    textInput->callback(updateValue, &table);
    textInput->when(FL_WHEN_ENTER_KEY);

    win.resizable(&tableArea);
    win.end();
    win.show();
    return Fl::run();
}