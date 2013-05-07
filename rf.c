#include <ncurses.h>
#include <form.h>
#include <string.h>
#include <stdlib.h>

WINDOW *newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
void menu(WINDOW *w);
void printmenu(WINDOW *menuwin, char **menu_s, int active, int chosen);
void home(WINDOW *w);
void reg(WINDOW *w);
void members(WINDOW *w);
void stats(WINDOW *w);


int main() {
  WINDOW *win;
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

  win = newwin(rows - 2, cols, y, x);
  keypad(win, true);
  refresh();
  box(win, 0, 0);
  wrefresh(win);

  char *RF = "REALISTFORENINGEN";
  mvprintw(0, (COLS - strlen(RF)) / 2, RF);
  mvprintw(1, 2, "Menu");
  home(win);
  menu(win);

  endwin();
  return 0;
}

void menu(WINDOW *w) {
  int active = 0, chosen = 0, i, ch;
  char **menu_s;
  WINDOW *menuwin;
  const int MENU_END = 4, MENU_LEN = 5;

  menu_s = malloc(sizeof(char*) * MENU_LEN);
  for (i = 0; i < MENU_LEN; i++)
    menu_s[i] = malloc(sizeof(char) * 15);
  menu_s[0] = "Home";
  menu_s[1] = "Register";
  menu_s[2] = "Members";
  menu_s[3] = "Stats";
  menu_s[4] = "Exit";

  menuwin = newwin(MENU_LEN + 2, 12, 2, 0);
  keypad(menuwin, true);
  box(menuwin, 0, 0);
  
  for (;;) {
    printmenu(menuwin, menu_s, active, chosen);
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
      printmenu(menuwin, menu_s, active, chosen);
      switch (active) {
      case 0:
        home(w);
        break;
      case 1:
        reg(w);
        break;
      case 2:
        members(w);
        break;
      case 3:
        stats(w);
        break;
      case 4:
        return;
      }
      break;
    default:
      mvprintw(23, 3, "Character pressed = %3d/%c", ch, ch);
      clrtoeol();
      refresh();
    }
  }
  for (i = 0; i < MENU_LEN; i++)
    free(menu_s[i]);
  free(menu_s);
}

void printmenu(WINDOW *mw, char **menu_s, int active, int chosen) {
  int y = 1, x = 1, i;
  box(mw, 0, 0);
  for (i = 0; i < 5; i++) {
    if (i == chosen)
      wattron(mw, A_BOLD);
    if (i == active) {
      wattron(mw, A_REVERSE);
      mvwprintw(mw, y++, x, menu_s[i]);
      wattroff(mw, A_REVERSE);
    } else {
      mvwprintw(mw, y++, x, menu_s[i]);
    }
    if (i == chosen)
      wattroff(mw, A_BOLD);
  }
  wrefresh(mw);
}

void home(WINDOW *w) {
  int x, y, ch;
  getmaxyx(w, y, x);
  char *s = "Dette er Realistforeningens medlemsliste.";
  werase(w);
  box(w, 0, 0);
  mvwprintw(w, y / 2, (x - strlen(s)) / 2, s);
  wrefresh(w);
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

void reg(WINDOW *w) {
  int x, y, ch, h = 15, wi = 50, i;
  FIELD *fields[4];
  FORM *rf_form;
  WINDOW *formw, *dwin;
  getmaxyx(w, y, x);

  for (i = 0; i < 3; i++) {
    fields[i] = new_field(1, 25, 2 + i * 2, 20, 0, 0);    
    set_field_back(fields[i], A_UNDERLINE);
  }
  fields[3] = NULL;

  rf_form = new_form(fields);
  scale_form(rf_form, &h, &wi);
  formw = newwin(h + 4, wi + 10, (y - h) / 2, (x - wi) / 2);
  keypad(formw, true);
  set_form_win(rf_form, formw);
  dwin = derwin(formw, h + 2, wi + 8, 1, 1);
  set_form_sub(rf_form, dwin);
  box(formw, 0, 0);
  post_form(rf_form);
  mvwprintw(dwin, 2, 4, "  Fornavn:");
  mvwprintw(dwin, 4, 4, "Etternavn:");
  mvwprintw(dwin, 6, 4, "       ID:");
  wrefresh(formw);

  int afi = 0;
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
    case 27:
      unpost_form(rf_form);
      free_form(rf_form);
      curs_set(0);
      for (i = 0; i < 3; i++)
        free_field(fields[i]);
      return;
    default:
      form_driver(rf_form, ch);
      break;
    }
    wrefresh(formw);
  }
}

void members(WINDOW *w) {
  return;
}

void stats(WINDOW *w) {
  int x, y, ch;
  getmaxyx(w, y, x);
  char *s = "Her kommer statistikk.";
  werase(w);
  box(w, 0, 0);
  mvwprintw(w, y / 2, (x - strlen(s)) / 2, s);
  wrefresh(w);
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}
