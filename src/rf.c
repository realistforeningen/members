#define _GNU_SOURCE
#define _X_OPEN_SOURCE_EXTENDED
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <form.h>
#include <panel.h>

int isspace(int ch);
void destroy_win(WINDOW *local_win);
WINDOW *newwin(int height, int width, int starty, int startx);

bool delete(char *needle, int dl);
char *strstrip(char *str);
int read_file();
void dump_to_file();
void home();
void members();
void menu();
void printmenu(WINDOW *mw, char **menu_s, int ML, int active);
void reg();
void register_member(char *f_name, char *l_name);
void search(char *needle, int hl);
void stats();
void update_status_line();

typedef struct member {
  char *first_name;
  char *last_name;
  long int timestamp;
  struct member *next;
} member;

const int PRICE = 50;
const char *RF = "REALISTFORENINGEN";
char* file_name= "members.csv";
int num_members, num_members_today, curr_line, curr_scroll;
member *first_member = NULL;
PANEL *panels[5];
WINDOW *main_win, *menu_win, *edit_win, *padw;

int main() {
  setlocale(LC_ALL, "");
  initscr();
  raw();
  keypad(stdscr, true);
  curs_set(0);
  noecho();
  clear();

  main_win = newwin(LINES - 2, COLS, 2, 0);
  keypad(main_win, true);
  refresh();
  box(main_win, 0, 0);
  wrefresh(main_win);
  
  ESCDELAY = 0;

  mvprintw(0, (COLS - strlen(RF)) / 2, RF);
  mvprintw(1, 2, "Menu  Edit");
  num_members = read_file();
  update_status_line();
  home();
  menu();

  endwin();
  return 0;
}

void menu() {
  int active_y = 0, active_x = 0, i, ch, cur_menu_len;
  char **menu_s, **menu_e;
  const int MENU_LEN = 5, EDIT_MENU_LEN = 3;

  panels[0] = new_panel(main_win);

  menu_s = malloc(sizeof(char*) * MENU_LEN);
  for (i = 0; i < MENU_LEN; i++)
    menu_s[i] = malloc(sizeof(char) * 15);
  menu_s[0] = " Home     ";
  menu_s[1] = " Register ";
  menu_s[2] = " Members  ";
  menu_s[3] = " Stats    ";
  menu_s[4] = " Exit     ";
  menu_e = malloc(sizeof(char*) * EDIT_MENU_LEN);
  for (i = 0; i < EDIT_MENU_LEN; i++)
    menu_e[i] = malloc(sizeof(char) * 15);
  menu_e[0] = " Backup   ";
  menu_e[1] = " N/A      ";
  menu_e[2] = " N/A      ";

  // Windows for roll-down menus
  menu_win = newwin(MENU_LEN + 2, 12, 2, 0);
  panels[1] = new_panel(menu_win);
  edit_win = newwin(EDIT_MENU_LEN + 2, 12, 2, 6);
  panels[4] = new_panel(edit_win);

  padw = newpad(5000, COLS - 2);
  panels[2] = new_panel(padw);
  update_panels();
  doupdate();

  keypad(menu_win, true);
  box(menu_win, 0, 0);
  keypad(edit_win, true);
  box(edit_win, 0, 0);

  for (;;) {
    if (active_x == 0) {
      hide_panel(panels[4]);
      show_panel(panels[1]);
      printmenu(menu_win, menu_s, MENU_LEN, active_y);
      //      printmenu(edit_win, menu_e, EDIT_MENU_LEN, active_y);
      cur_menu_len = MENU_LEN;
    } else {
      hide_panel(panels[1]);
      show_panel(panels[4]);
      //      printmenu(menu_win, menu_s, MENU_LEN, active_y);
      printmenu(edit_win, menu_e, EDIT_MENU_LEN, active_y);
      cur_menu_len = EDIT_MENU_LEN;
    }
    update_panels();
    doupdate();

    switch (ch = getch()) {
    case 27:
      return;
    case KEY_RIGHT:
    case KEY_LEFT:
      //      update_panels();
      //      doupdate();
      active_y = 0;
      active_x = active_x == 0 ? 1 : 0;
      break;
    case KEY_UP:
      active_y == 0 ? active_y = cur_menu_len - 1 : active_y--;
      break;
    case KEY_DOWN:
      active_y == cur_menu_len - 1 ? active_y = 0 : active_y++;
      break;
    case 10:
      switch (active_y) {
      case 0:
        active_x ? 0 : home();
        break;
      case 1:
        active_x ? 0 : reg();
        break;
      case 2:
        active_x ? 0 : members();
        break;
      case 3:
        stats();
        break;
      case 4:
        return;
      }
    }
  }
  for (i = 0; i < MENU_LEN; i++)
    free(menu_s[i]);
  free(menu_s);
  for (i = 0; i < EDIT_MENU_LEN; i++)
    free(menu_e[i]);
  free(menu_e);
}

void printmenu(WINDOW *mw, char **menu_s, int ML, int active) {
  int y = 1, x = 1, i;
  box(mw, 0, 0);
  for (i = 0; i < ML; i++) {
    if (i == active) {
      wattron(mw, A_REVERSE);
      mvwprintw(mw, y++, x, menu_s[i]);
      wattroff(mw, A_REVERSE);
    } else {
      mvwprintw(mw, y++, x, menu_s[i]);
    }
  }
  //wrefresh(mw);
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

void reg() {
  int x, y, ch, h = 15, wi = 50, i;
  FIELD *fields[3];
  FORM *rf_form;
  WINDOW *formw, *dwin;
  char *f_name, *l_name;
  hide_panel(panels[1]);
  getmaxyx(main_win, y, x);

  for (i = 0; i < 2; i++) {
    fields[i] = new_field(1, 25, 1 + i * 2, 12, 0, 0);    
    set_field_back(fields[i], A_UNDERLINE);
  }
  fields[2] = NULL;

  rf_form = new_form(fields);
  scale_form(rf_form, &h, &wi);
  formw = newwin(h + 3, wi + 4, (y - h) / 2, (x - wi) / 2);
  panels[3] = new_panel(formw);
  update_panels();
  doupdate();
  keypad(formw, true);
  set_form_win(rf_form, formw);
  dwin = derwin(formw, h, wi + 2, 1, 1);
  keypad(dwin, true);
  set_form_sub(rf_form, dwin);
  box(formw, 0, 0);
  post_form(rf_form);
  mvwprintw(dwin, 1, 1, "  Fornavn:");
  mvwprintw(dwin, 3, 1, "Etternavn:");
  //  prefresh(padw, curr_line, 1, 3, 1, y, x-2);
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
      form_driver(rf_form, REQ_NEXT_FIELD);
      f_name = strstrip(field_buffer(fields[0], 0));
      l_name = strstrip(field_buffer(fields[1], 0));
      register_member(f_name, l_name);
    case 27:
      hide_panel(panels[3]);
      //      if (ch == 27)
        //        show_panel(panels[1]);
      update_panels();
      doupdate();
      //      prefresh(padw, curr_line, 1, 3, 1, y, x-2);
      unpost_form(rf_form);
      free_form(rf_form);
      curs_set(0);
      for (i = 0; i < 2; i++)
        free_field(fields[i]);
      return;
    default:
      form_driver(rf_form, ch);
      break;
    }
    wrefresh(formw);
  }
}

void members() {
  int x, y, ch;
  hide_panel(panels[1]);
  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);
  update_panels();
  doupdate();

  search("", 0);
  bool search_mode = false;
  char *needle_buf = "", *send_s;
  int needle_idx = 0;
  bool btm = false;
  curr_line = 0;
  curr_scroll = 0;
  prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
  for (;;) {
    switch (ch = getch()) {
    case KEY_DOWN:
      if (curr_line == num_members - 1) {
        btm = false;
        break;}
      search(needle_buf, ++curr_line);
      if (curr_line == num_members - 1 && btm)
        break;
      if (curr_line > y - 3)
        prefresh(padw, ++curr_scroll, 1, 3, 1, y, x-2);
      move(1, 27 + needle_idx);
      break;
    case KEY_UP:
      if (curr_line == 0) {
        btm = false;
        break;
      }
      if (curr_line == num_members - 1)
        btm = true;
      search(needle_buf, --curr_line);
      if (curr_line < curr_scroll)
        prefresh(padw, --curr_scroll, 1, 3, 1, y, x-2);
      move(1, 27 + needle_idx);
      break;
    case KEY_BACKSPACE:
      if (search_mode && needle_idx > 0) {
        needle_buf[--needle_idx] = '\0';
        send_s = (char *) malloc(needle_idx + 1);
        strncpy(send_s, needle_buf, needle_idx+1);
        search(send_s, 0);
        free(send_s);
        mvprintw(1, 26, "                         ");
        mvprintw(1, 27, "%s", needle_buf);
      }
      break;
    case KEY_DC:
      if (!delete(needle_buf, curr_line))
        break;
      search(needle_buf, 0);
      num_members--;
      dump_to_file();
      update_status_line();
      break;
    case 47:
      if (!search_mode) {
        curr_line = 0;
        curr_scroll = 0;
        search_mode = true;
        needle_buf = (char *) malloc(100);
        mvprintw(1, 19, "Search:");
        curs_set(1);
        move(1, 27);
      }
      break;
    case 27:
      if (search_mode) {
        search_mode = false;
        needle_idx = 0;
        curs_set(0);
        werase(padw);
        search("", 0);
        free(needle_buf);
        mvprintw(1, 19, "                                ");
        prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
        break;
      }
      werase(padw);
      prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
      return;
    default:
      //      mvprintw(1, 19,"%d", ch);
      if (search_mode && needle_idx < 32) {
        werase(padw);
        needle_buf[needle_idx++] = (char) ch;
        needle_buf[needle_idx] = '\0';
        send_s = (char *) malloc(needle_idx + 1);
        strncpy(send_s, needle_buf, needle_idx + 1);
        search(send_s, 0);
        free(send_s);
        prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
        mvprintw(1, 26, " %s", needle_buf);
      }
    }
  }
}

bool delete(char *needle, int dl) {
  int i;
  member *curr = first_member;
  member *prev = first_member;
  for (i = 0; curr != NULL;) {
    if (strcasestr(curr->first_name, needle) ||
        strcasestr(curr->last_name, needle)) {
      if (i++ == dl) {
        prev->next = curr->next;
        return true;
      }
    }
    prev = curr;
    curr = curr->next;
  }
  return false;
}

void search(char *needle, int hl) {
  int i, y, x;
  member *curr = first_member;
  getmaxyx(main_win, y, x);
  werase(padw);
  for (i = 0; curr != NULL;) {
    if (strcasestr(curr->first_name, needle) ||
        strcasestr(curr->last_name, needle)) {
      i == hl ? wattron(padw, A_REVERSE) : 0;
      mvwprintw(padw, i, 2, "%s %s", curr->first_name, curr->last_name);
      mvwprintw(padw, i, x - 26, "%s", asctime(localtime(&curr->timestamp)));
      i == hl ? wattroff(padw, A_REVERSE) : 0;
      i++;
    }
    curr = curr->next;
  }
  prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
}

char *strstrip(char *str) {
  char *end;
  while (isspace(*str))
    str++;
  if (*str == 0)
    return str;
  end = str + strlen(str) - 1;
  while (end > str && isspace(*end))
    end--;
  *(end + 1) = 0;
  return str;
}

void stats() {
  int x, y, ch;
  char *s = "Her kommer statistikk.";
  getmaxyx(main_win, y, x);
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

void register_member(char *f_name, char *l_name) {
  member *tmp = (member *) malloc(sizeof(member));
  tmp->first_name = (char *) malloc(strlen(f_name) * sizeof(char));
  strcpy(tmp->first_name, f_name);
  tmp->last_name = (char *) malloc(strlen(l_name) * sizeof(char));
  strcpy(tmp->last_name, l_name);
  tmp->timestamp = time(NULL);
  if (first_member == NULL) {
    first_member = tmp;
  } else {
    tmp->next = first_member;
    first_member = tmp;
  }
  dump_to_file();
  num_members_today++;
  num_members++;
  update_status_line();
}

void update_status_line() {
  mvprintw(1, COLS - 33, "NOK: %5d TDY: %5d TTL: %5d",
           num_members_today * 30, num_members_today, num_members);
}

void dump_to_file() {
  FILE *fp = fopen(file_name, "w");
  member *curr = first_member;
  while (curr != NULL) {
    fprintf(fp, "%s,%s,%ld\n", curr->last_name,
            curr->first_name, curr->timestamp);
    curr = curr->next;
  }
  fclose(fp);
}

member *parseline(char *line) {
  char *l_name, *f_name;
  long int ts;

  l_name = strtok(line, ",");
  f_name = strtok(NULL, ",\n");
  ts =  strtol(strtok(NULL, ",\n"), NULL, 10);

  member *tmp = (member *) malloc(sizeof(member));
  tmp->first_name = (char *) malloc(strlen(f_name) * sizeof(char));
  tmp->last_name = (char *) malloc(strlen(l_name) * sizeof(char));

  tmp->first_name = strdup(f_name);
  tmp->last_name = strdup(l_name);
  tmp->timestamp = ts;
  return tmp;
}

int read_file() {
  FILE *fp = fopen(file_name, "r");
  char line[1024];
  int i = 0;
  while (fgets(line, 1024, fp)) {
    member *tmp = parseline(strdup(line));
    if (first_member == NULL) {
      first_member = tmp;
    } else {
      tmp->next = first_member;
      first_member = tmp;
    }
    i++;
  }
  fclose(fp);
  return i;
}
