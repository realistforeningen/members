#include <ncurses.h>
#include <form.h>
#include <panel.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

WINDOW *newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
void menu();
void printmenu(char **menu_s, int active, int chosen);
void home();
bool reg();
void members();
void stats();

const int PRICE = 30;
const char *RF = "REALISTFORENINGEN";
WINDOW *main_win;
WINDOW *menu_win;
PANEL *panels[3];

int main() {
  setlocale(LC_ALL, "");

  initscr();
  raw();
  keypad(stdscr, true);
  curs_set(0);
  noecho();
  clear();

  int rows, cols;
  char *s;

  getmaxyx(stdscr, rows, cols);
  int y = 2;
  int x = 0;

  main_win = newwin(rows - 2, cols, y, x);
  keypad(main_win, true);
  refresh();
  box(main_win, 0, 0);
  wrefresh(main_win);

  mvprintw(0, (COLS - strlen(RF)) / 2, RF);
  mvprintw(1, 2, "Menu");
  home();
  menu();

  endwin();
  return 0;
}

void menu() {
  int active = 0, chosen = 0, gomenu, i, ch;
  char **menu_s;
  const int MENU_END = 4, MENU_LEN = 5;

  panels[0] = new_panel(main_win);

  menu_s = malloc(sizeof(char*) * MENU_LEN);
  for (i = 0; i < MENU_LEN; i++)
    menu_s[i] = malloc(sizeof(char) * 15);
  menu_s[0] = "Home";
  menu_s[1] = "Register";
  menu_s[2] = "Members";
  menu_s[3] = "Stats";
  menu_s[4] = "Exit";

  menu_win = newwin(MENU_LEN + 2, 12, 2, 0);
  panels[1] = new_panel(menu_win);
  update_panels();
  doupdate();
  keypad(menu_win, true);
  box(menu_win, 0, 0);
  
  for (;;) {
    printmenu(menu_s, active, chosen);
    switch (ch = getch()) {
    case 27:
      return;
    case KEY_UP:
      active == 0 ? active = MENU_END : active--;
      break;
    case KEY_DOWN:
      active == MENU_END ? active = 0 : active++;
      break;
    case 10:
      chosen = active;
      switch (active) {
      case 0:
        home();
        break;
      case 1:
        hide_panel(panels[1]);
        update_panels();
        doupdate();
        if (!reg(panels))
          break;
      case 2:
        members(panels);
        break;
      case 3:
        stats();
        break;
      case 4:
        return;
      }
      show_panel(panels[1]);
      update_panels();
      doupdate();
    }
  }
  for (i = 0; i < MENU_LEN; i++)
    free(menu_s[i]);
  free(menu_s);
}

void printmenu(char **menu_s, int active, int chosen) {
  int y = 1, x = 1, i;
  box(menu_win, 0, 0);
  for (i = 0; i < 5; i++) {
    if (i == chosen)
      wattron(menu_win, A_BOLD);
    if (i == active) {
      wattron(menu_win, A_REVERSE);
      mvwprintw(menu_win, y++, x, menu_s[i]);
      wattroff(menu_win, A_REVERSE);
    } else {
      mvwprintw(menu_win, y++, x, menu_s[i]);
    }
    if (i == chosen)
      wattroff(menu_win, A_BOLD);
  }
  wrefresh(menu_win);
}

void home() {
  // TODO dancing bear
  int x, y, ch;
  getmaxyx(main_win, y, x);
  char *s = "Dette er Realistforeningens medlemsliste.";
  werase(main_win);
  box(main_win, 0, 0);
  mvwprintw(main_win, y / 2, (x - strlen(s)) / 2, s);
  wrefresh(main_win);
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

bool reg() {
  int x, y, ch, h = 15, wi = 50, i;
  FIELD *fields[4];
  FORM *rf_form;
  WINDOW *formw, *dwin;
  char *f_name, *l_name;
  unsigned int uid;
  getmaxyx(main_win, y, x);

  wrefresh(main_win);

  for (i = 0; i < 3; i++) {
    fields[i] = new_field(1, 25, 1 + i * 2, 12, 0, 0);    
    set_field_back(fields[i], A_UNDERLINE);
  }
  fields[3] = NULL;

  rf_form = new_form(fields);
  scale_form(rf_form, &h, &wi);
  formw = newwin(h + 3, wi + 4, (y - h) / 2, (x - wi) / 2);
  panels[2] = new_panel(formw);
  update_panels();
  doupdate();
  keypad(formw, true);
  set_form_win(rf_form, formw);
  dwin = derwin(formw, h, wi + 2, 1, 1);
  set_form_sub(rf_form, dwin);
  box(formw, 0, 0);
  post_form(rf_form);
  mvwprintw(dwin, 1, 1, "  Fornavn:");
  mvwprintw(dwin, 3, 1, "Etternavn:");
  mvwprintw(dwin, 5, 1, "       ID:");
  wrefresh(formw);

  curs_set(1);
  for (;;) {
    switch (ch = getch()) {
    case KEY_UP:
    case KEY_BTAB:
      form_driver(rf_form, REQ_PREV_FIELD);
      form_driver(rf_form, REQ_END_LINE);
      break;
    case KEY_DOWN:
    case 9:
      form_driver(rf_form, REQ_NEXT_FIELD);
      form_driver(rf_form, REQ_END_LINE);
      break;
    case KEY_LEFT:
      form_driver(rf_form, REQ_PREV_CHAR);
      break;
    case KEY_RIGHT:
      form_driver(rf_form, REQ_NEXT_CHAR);
      break;
    case KEY_BACKSPACE:
      form_driver(rf_form, REQ_DEL_PREV);
      break;
    case 10:
      f_name = field_buffer(fields[0], 0);
      l_name = field_buffer(fields[1], 0);
      uid = strtoul(field_buffer(fields[2], 0), NULL, 10);
      // TODO save data
    case 27:
      hide_panel(panels[2]);
      if (ch == 27)
        show_panel(panels[1]);
      update_panels();
      doupdate();
      unpost_form(rf_form);
      free_form(rf_form);
      curs_set(0);
      for (i = 0; i < 3; i++)
        free_field(fields[i]);
      return ch == 10;
    default:
      form_driver(rf_form, ch);
      break;
    }
    wrefresh(formw);
  }
}

void members() {
  hide_panel(panels[1]);
  update_panels();
  doupdate();
  int x, y, ch;
  getmaxyx(main_win, y, x);
  char *s = "Her kommer medlemslista.";
  werase(main_win);
  box(main_win, 0, 0);
  mvwprintw(main_win, y / 2, (x - strlen(s)) / 2, s);
  wrefresh(main_win);
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

void stats() {
  int x, y, ch;
  getmaxyx(main_win, y, x);
  char *s = "Her kommer statistikk.";
  werase(main_win);
  box(main_win, 0, 0);
  mvwprintw(main_win, y / 2, (x - strlen(s)) / 2, s);
  wrefresh(main_win);
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}
