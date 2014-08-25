#define _GNU_SOURCE
#define _X_OPEN_SOURCE_EXTENDED
#include <locale.h>
#include <string.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <form.h>
#include <panel.h>
#include <sqlite3.h>
#include <ctype.h>

#define IN_BACKGROUND 1
#define IN_FOREGROUND 0
#define WAIT          1
#define NOWAIT        0

bool delete(sqlite3 *db, WINDOW *main_win, PANEL **panels, int dl);
bool dialog_sure(WINDOW *main_win, PANEL **panels);
char *strstrip(char *str);
char *strtok2(char *line, char tok);
int csv2reg(char *line);
int get_lifetimers();
int search(sqlite3 *db, WINDOW *main_win, WINDOW *padw,
           char *needle, long period_begin, long period_end,
           int *curr_line, int *visible_members, int *delete_rowid,
           int curr_scroll);
int ssh_backup(WINDOW *backupw);
int ssh_fetch_db(WINDOW *main_win);
int *sem2my(char *sem);
void backup(WINDOW *main_win, PANEL **panels);
void debug(char *msg);
void fetch_db(WINDOW *main_win, PANEL **panels);
void home(WINDOW *main_win, int w);
void member_help();
void members(sqlite3 *db, WINDOW *main_win, WINDOW *padw, PANEL **panels,
             int bg, long period_begin, long period_end,
             int *curr_scroll, int price);
void menu(sqlite3 *db, WINDOW *main_win,
          PANEL **panels, long semstart, int price);
void print_y_n(WINDOW *w, int active);
void printmenu(WINDOW *mw, char **menu_s, int ML, int active);
void read_conf(char *conf_name, int *price, char *domain,
               char *path, char *file_name);
void refresh_bg(sqlite3 *db, WINDOW *main_win, WINDOW *padw, int prev_y,
                int curr_scroll, long semstart, int price);
void reg(sqlite3 *db, WINDOW *main_win, WINDOW *padw, PANEL **panels,
         int curr_scroll, int curr_line, int visible_members,
         int delete_rowid, long semstart, int price);
void register_member(sqlite3 *db, WINDOW *main_win, char *f_name,
                     char *l_name, bool lifetime, long ts, long semstart,
                     int price);
void stats(sqlite3 *db, WINDOW *main_win, int bg, long semstart,
           int price, int w);

typedef struct {
  WINDOW *padw;
  int *curr, *curr_line, *visible_members, *delete_rowid;
} callback_container;

int main() {
  int price;
  long semstart;
  WINDOW *main_win;
  sqlite3 *db;
  PANEL *panels[4];
  char _[20];

  read_conf("rf.conf", &price, _, _, _);

  // Set up ncurses
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
  ESCDELAY = 0;
  wrefresh(main_win);

  /* start_color();
     assume_default_colors(3, 0); */

  const char *RF = "R E A L I S T F O R E N I N G E N";
  attron(A_BOLD);
  mvprintw(0, (COLS - strlen(RF)) >> 1, RF);
  mvprintw(1, 2, "Menu");
  attroff(A_BOLD);
  refresh();

  // Define start of semester
  time_t ts_now = time(NULL);
  struct tm *now = gmtime(&ts_now);
  now->tm_hour = 0;
  now->tm_min = 0;
  now->tm_sec = 0;
  now->tm_mday = 1;
  now->tm_mon = now->tm_mon < 6 ? 0 : 6;
  semstart = mktime(now);

  // Fetch database
  //  ssh_fetch_db(main_win);

  // Open database
  sqlite3_open("members.db", &db);
  char *errmsg;
  char *sql = "CREATE TABLE IF NOT EXISTS members\
 (first_name NCHAR(50) NOT NULL, last_name NCHAR(50) NOT NULL,\
 lifetime TINYINT, timestamp BIGINT NOT NULL, paid INT)";
  sqlite3_exec(db, sql, NULL, NULL, &errmsg);
  stats(db, main_win, IN_BACKGROUND, semstart, price, WAIT);

  // Start main loop
  home(main_win, WAIT);
  menu(db, main_win, panels, semstart, price);

  delwin(main_win);
  endwin();
  return 0;
}

void menu(sqlite3 *db, WINDOW *main_win,
          PANEL **panels, long semstart, int price) {
  int active_y = 0, prev_y = 0, ch;
  char **menu_s;
  WINDOW *menu_win, *padw;
  const int MENU_LEN = 5;
  int curr_scroll = 0, x, y;

  panels[0] = new_panel(main_win);

  menu_s = malloc(sizeof(char*) * MENU_LEN);
  menu_s[0] = " Home     ";
  menu_s[1] = " Members  ";
  menu_s[2] = " Stats    ";
  menu_s[3] = " Backup   ";
  menu_s[4] = " Exit     ";

  // Windows for roll-down menus
  menu_win = newwin(MENU_LEN + 2, 12, 2, 0);
  panels[1] = new_panel(menu_win);

  padw = newpad(5000, COLS - 2);
  panels[2] = new_panel(padw);

  getmaxyx(main_win, y, x);
  keypad(menu_win, true);
  box(menu_win, 0, 0);

  for (;;) {
    getmaxyx(main_win, y, x);
    show_panel(panels[1]);
    printmenu(menu_win, menu_s, MENU_LEN, active_y);
    update_panels();
    doupdate();
    
    switch (ch = getch()) {
    case 27:
      if (dialog_sure(main_win, panels)) {
        free(menu_s);
        delwin(menu_win);
        delwin(padw);
        return;
      }
      refresh_bg(db, main_win, padw, prev_y, curr_scroll, semstart, price);
      break;
    case KEY_UP:
      active_y == 0 ? active_y = MENU_LEN - 1 : active_y--;
      break;
    case KEY_DOWN:
      active_y == MENU_LEN - 1 ? active_y = 0 : active_y++;
      break;
    case 10:
      switch (active_y) {
      case 0:
        home(main_win, WAIT);
        break;
      case 1:
        members(db, main_win, padw, panels, IN_FOREGROUND,
                semstart, (long) time(NULL), &curr_scroll, price);
        break;
      case 2:
        stats(db, main_win, IN_FOREGROUND, semstart, price, WAIT);
        break;
      case 3:
        backup(main_win, panels);
        refresh_bg(db, main_win, padw, prev_y, curr_scroll, semstart, price);
        break;
      case 4:
        if (dialog_sure(main_win, panels)) {
          free(menu_s);
          delwin(menu_win);
          delwin(padw);
          return;
        }
        refresh_bg(db, main_win, padw, prev_y, curr_scroll, semstart, price);
      }
      prev_y = active_y != 4
        && active_y != 5 ? active_y : prev_y;
    }
  }
}

void refresh_bg(sqlite3 *db, WINDOW *main_win, WINDOW *padw, int prev_y,
                int curr_scroll, long semstart, int price) {
  int x, y;
  getmaxyx(main_win, y, x);
  switch (prev_y) {
  case 0:
    home(main_win, NOWAIT);
    break;
  case 1:
    prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
    break;
  case 2:
    stats(db, main_win, IN_FOREGROUND, semstart, price, NOWAIT);
    break;
  }
}

void printmenu(WINDOW *mw, char **menu_s, int ML, int active) {
  int y = 1, x = 1, i;
  box(mw, 0, 0);
  for (i = 0; i < ML; i++) {
    i == active ? wattron(mw, A_REVERSE) : 0;
    mvwprintw(mw, y++, x, menu_s[i]);
    i == active ? wattroff(mw, A_REVERSE) : 0;
  }
  wrefresh(mw);
}

void home(WINDOW *main_win, int w) {
  int i;
  char *bear[27];
  int x, y, ch;

  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);

  bear[0] = "                         _____";
  bear[1] = "                      _d       b_";
  bear[2] = "                   _d             b_";
  bear[3] = "                 _d                 b_";
  bear[4] = "              _d                      b_";
  bear[5] = "         (\\____ /\\                      b";
  bear[6] = "         /      a )                      b";
  bear[7] = "        ( Ø  Ø                            b";
  bear[8] = "     d888b       _  \\                       b";
  bear[9] = "     qO8Op      ´|` |                        b";
  bear[10] = "      (_____,--´´  /                         b";
  bear[11] = "          (___,---´                           8";
  bear[12] = " ________    \\88\\/88                           8";
  bear[13] = "(____  / \\..__88/\\88          |         |      8";
  bear[14] = "   (__(_      |          _____|__       |      8";
  bear[15] = "    (___     /        ,/´        `\\     |      8";
  bear[16] = "     (__)____|       /                  /      8";
  bear[17] = "             \\      ///                /       8";
  bear[18] = "             |      ||| | ___         /        p";
  bear[19] = "             |      `\\\\_\\/   `\\_____,/        |";
  bear[20] = "             |                               |";
  bear[21] = "             |                               |";
  bear[22] = "             \\            __              /\\ (";
  bear[23] = "             /           / /            ./  \\";
  bear[24] = "            |           / (            (";
  bear[25] = "          __|,.,.       |_|,.,.        \\\\";
  bear[26] = "         ((_(_(_,__,___,((_(_(_,__,_____)";

  for (i = 0; i < 27; i++)
    mvwprintw(main_win, (y >> 1) - 15 + i,
              ((x - strlen(bear[12])) >> 1) - 3, bear[i]);

  char *s = "Dette er Realistforeningens medlemsliste.";
  mvwprintw(main_win, (y >> 1) - 14 + i, (x - strlen(s)) >> 1, s);
  wrefresh(main_win);
  if (w == NOWAIT)
    return;
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

void reg(sqlite3 *db, WINDOW *main_win, WINDOW *padw, PANEL **panels,
         int curr_scroll, int curr_line, int visible_members,
         int delete_rowid, long semstart, int price) {
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
  formw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
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
  wmove(dwin, 1, 12);
  wrefresh(formw);
  curs_set(1);
  for (;;) {
    switch (ch = wgetch(dwin)) {
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
      if (!(*f_name & *l_name))
        break; // Don't allow empty names
      register_member(db, main_win, f_name, l_name, false, time(NULL),
                      semstart, price);
      search(db, main_win, padw, "", semstart, (long) time(NULL),
             &curr_line, &visible_members, &delete_rowid, curr_scroll);
      prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
      box(formw, 0, 0);
      form_driver(rf_form, REQ_CLR_FIELD);
      form_driver(rf_form, REQ_NEXT_FIELD);
      form_driver(rf_form, REQ_CLR_FIELD);
      form_driver(rf_form, REQ_NEXT_FIELD);
      mvwprintw(dwin, 1, 1, "  Fornavn:");
      mvwprintw(dwin, 3, 1, "Etternavn:");
      wmove(dwin, 1, 12);
      wrefresh(formw);
      break;
    case 27:
      hide_panel(panels[3]);
      update_panels();
      doupdate();
      unpost_form(rf_form);
      free_form(rf_form);
      curs_set(0);
      for (i = 0; i < 2; i++)
        free_field(fields[i]);
      prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
      delwin(formw);
      return;
    default:
      form_driver(rf_form, ch);
      break;
    }
    wrefresh(formw);
  }
}

void members(sqlite3 *db, WINDOW *main_win, WINDOW *padw, PANEL **panels,
             int bg, long period_begin, long period_end, int *curr_scroll,
             int price) {
  int x, y, ch;
  hide_panel(panels[1]);
  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);
  update_panels();
  doupdate();
  int needle_idx = 0, curr_line = 0, visible_members = 0;
  int delete_rowid = 0;

  *curr_scroll = 0;
  search(db, main_win, padw, "", period_begin, period_end, &curr_line,
         &visible_members, &delete_rowid, *curr_scroll);
  bool search_mode = false;
  char *send_s, *needle_buf = malloc(sizeof(char) * 100);
  needle_buf[0] = '\0';

  bool btm = false; // Hack to make scrolling work
  prefresh(padw, *curr_scroll, 1, 3, 1, y, x-2);
  if (bg)
    return;
  for (;;) {
    if (search_mode) {
      switch (ch = getch()) {
      case 27:
          search_mode = false;
          needle_idx = 0;
          curs_set(0);
          werase(padw);
          search(db, main_win, padw, "", period_begin, (long) time(NULL),
                 &curr_line, &visible_members, &delete_rowid, *curr_scroll);
          mvprintw(1, 19, "                                ");
          prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
          break;
      case KEY_BACKSPACE:
        if (needle_idx > 0) {
          needle_buf[--needle_idx] = '\0';
          send_s = (char *) malloc(needle_idx + 1);
          strncpy(send_s, needle_buf, needle_idx+1);
          curr_line = 0;
          search(db, main_win, padw, send_s, period_begin, (long) time(NULL),
                 &curr_line, &visible_members, &delete_rowid, *curr_scroll);
          free(send_s);
          mvprintw(1, 26, "                         ");
          mvprintw(1, 27, "%s", needle_buf);
        }
        break;
      default:
        if (search_mode && needle_idx < 32) {
          werase(padw);
          needle_buf[needle_idx++] = (char) ch;
          needle_buf[needle_idx] = '\0';
          send_s = (char *) malloc(needle_idx + 1);
          strncpy(send_s, needle_buf, needle_idx + 1);
          search(db, main_win, padw, send_s, period_begin, (long) time(NULL),
                 &curr_line, &visible_members, &delete_rowid, *curr_scroll);
          free(send_s);
          prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
          mvprintw(1, 26, " %s", needle_buf);
        }
      }
    } else {
      switch (ch = getch()) {
      case 104: // H for Help
        member_help(main_win, panels);
        prefresh(padw, *curr_scroll, 1, 3, 1, y, x-2);
        break;
      case KEY_DOWN:
        if (curr_line == visible_members - 1) {
          btm = false;
          break;}
        curr_line++;
        search(db, main_win, padw, needle_buf, period_begin, (long) time(NULL),
               &curr_line, &visible_members, &delete_rowid, *curr_scroll);
        if (curr_line == visible_members - 1 && btm)
          break;
        if (curr_line > y - 3)
          prefresh(padw, ++(*curr_scroll), 1, 3, 1, y, x-2);
        move(1, 27 + needle_idx);
        break;
      case KEY_UP:
        if (curr_line == 0) {
          btm = false;
          break;
        }
        if (curr_line == visible_members - 1)
          btm = true;
        curr_line--;
        search(db, main_win, padw, needle_buf, period_begin, (long) time(NULL),
               &curr_line, &visible_members, &delete_rowid, *curr_scroll);
        if (curr_line < *curr_scroll)
          prefresh(padw, --(*curr_scroll), 1, 3, 1, y, x-2);
        move(1, 27 + needle_idx);
        break;
      case KEY_DC: // Delete
        if (!delete(db, main_win, panels, delete_rowid)) {
          prefresh(padw, *curr_scroll, 1, 3, 1, y, x-2);
          break;
        }
        stats(db, main_win, IN_BACKGROUND, period_begin, price, WAIT);
        curr_line = 0;
        search(db, main_win, padw, needle_buf, period_begin, (long) time(NULL),
               &curr_line, &visible_members, &delete_rowid, *curr_scroll);
        break;
      case 47:
        curr_line = 0;
        *curr_scroll = 0;
        search_mode = true;
        mvprintw(1, 19, "Search:");
        curs_set(1);
        move(1, 27);
        break;
      case 27:
        free(needle_buf);
        curr_line = 0;
        prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
        return;
      case 110: // N for New member
          reg(db, main_win, padw, panels, *curr_scroll, curr_line,
              visible_members, delete_rowid, period_begin, price);
          search(db, main_win, padw, "", period_begin, period_end, &curr_line,
                 &visible_members, &delete_rowid, *curr_scroll);
      }
    }
  }
}
  
void member_help(WINDOW *main_win, PANEL **panels) {
  int x, y, ch, h = 7, wi = 23, i = 1;
  WINDOW *dialogw;
  getmaxyx(main_win, y, x);

  dialogw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
  panels[3] = new_panel(dialogw);
  update_panels();
  doupdate();
  box(dialogw, 0, 0);

  wattron(dialogw, A_BOLD);
  mvwprintw(dialogw, i++, 2,   " HELP FOR KEY BINDINGS");
  wattroff(dialogw, A_BOLD);
  i++;
  mvwprintw(dialogw, i++, 2,   "H   - Opens this window");
  mvwprintw(dialogw, i++, 2,   "N   - Add new member");
  mvwprintw(dialogw, i++, 2,   "/   - Search by name");
  mvwprintw(dialogw, i++, 2,   "ESC - Close window or");
  mvwprintw(dialogw, i++, 2,   "      open main menu");


  wrefresh(dialogw);
  for (;;) {
    switch (ch = getch()) {
    case 27: // n
      hide_panel(panels[3]);
      update_panels();
      doupdate();
      delwin(dialogw);
      return;
    }
  }
}

static char *base36(long value) {
  char base36[37] = "0123456789ABCDEFGHiJKLMNoPQRSTUVWXYZ";
  /* log(2**64) / log(36) = 12.38 => max 13 char + '\0' */
  char buffer[14];
  unsigned int offset = sizeof(buffer);

  buffer[--offset] = '\0';
  do {
    buffer[--offset] = base36[value % 36];
  } while (value /= 36);

  return strdup(&buffer[offset]);
}

void print_y_n(WINDOW *w, int active) {
  int y = 2, x = 3, i;
  char *txt[] = {"    No     ", "    Yes    "};
  for (i = 0; i < 2; i++) {
    i == active ? wattron(w, A_REVERSE) : 0;
    mvwprintw(w, y++, x, txt[i]);
    i == active ? wattroff(w, A_REVERSE) : 0;
  }
  wrefresh(w);
}

bool dialog_sure(WINDOW *main_win, PANEL **panels) {
  int x, y, ch, h = 2, wi = 13, active = 0;
  WINDOW *dialogw;
  getmaxyx(main_win, y, x);  

  dialogw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
  panels[3] = new_panel(dialogw);
  update_panels();
  doupdate();
  box(dialogw, 0, 0);

  mvwprintw(dialogw, 1, 2, "Are you sure?");
  wrefresh(dialogw);
  print_y_n(dialogw, active);
  for (;;) {
    switch (ch = getch()) {
    case KEY_UP:
    case KEY_DOWN:
      print_y_n(dialogw, active = !active);
      break;
    case 121: // y
    case 10:
      if (active || ch == 121) {
      hide_panel(panels[3]);
      update_panels();
      doupdate();
      delwin(dialogw);
      return true;
      }
    case 110: // n
      hide_panel(panels[3]);
      update_panels();
      doupdate();
      delwin(dialogw);
      return false;
    }
  }
}

bool delete(sqlite3 *db, WINDOW *main_win, PANEL **panels, int dl) {
  if (!dialog_sure(main_win, panels)) {
    return false;
  }
  char sqls[50];
  sprintf(sqls, "DELETE FROM members WHERE rowid == %d AND\
 lifetime == 0", dl);
  sqlite3_exec(db, sqls, 0, 0, 0);
  return true;
}

void backup(WINDOW *main_win, PANEL **panels) {
  int x, y, h = 15, wi = 70;
  WINDOW *backupw;
  hide_panel(panels[1]);
  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);

  backupw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
  panels[3] = new_panel(backupw);
  update_panels();
  doupdate();
  box(backupw, 0, 0);

  ssh_backup(backupw);
  getch(); // Wait for any key

  hide_panel(panels[3]);
  update_panels();
  doupdate();
  delwin(backupw);
  return;
}

void fetch_db(WINDOW *main_win, PANEL **panels) {
  int x, y, h = 15, wi = 70;
  WINDOW *fetchw;
  hide_panel(panels[1]);
  getch();
  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);

  fetchw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
  panels[3] = new_panel(fetchw);
  update_panels();
  doupdate();
  box(fetchw, 0, 0);

  ssh_fetch_db(fetchw);
  getch(); // Wait for any key

  hide_panel(panels[3]);
  update_panels();
  doupdate();
  delwin(fetchw);
  return;
}

char *get_string(WINDOW *w, int y, int x, bool h) {
  char *pass, *rf, *ch_s;
  int ch, i = 0;
  pass = malloc(sizeof(char)*30);
  ch_s = malloc(sizeof(char)*2);
  curs_set(1);
  wmove(w, y, x);
  for (;;) {
    switch (ch = wgetch(w)) {
    case KEY_BACKSPACE:
      if (i) i--;
      mvwprintw(w, y, x + i, " ");
      wmove(w, y, x + i);
      wrefresh(w);
      break;
    case 10:
      curs_set(0);
      pass[i] = '\0';
      free(ch_s);
      return pass;
    default:
      if (i > 30)
        break;
      pass[i] = ch;
      rf = i % 2 ? "F" : "R";
      sprintf(ch_s, "%c", ch);
      mvwprintw(w, y, x + i++, h ? rf : ch_s);
      wrefresh(w);
      break;
    }
  }
}

int ssh_backup(WINDOW *backupw) {
  ssh_session sshs = ssh_new();
  ssh_scp scp;
  int rc, size, prev_tmp, line = 1, _;
  char *user, domain[80], *password, path[100], *buffer, tmp[300];
  char file_name[80];
  FILE *member_file;

  read_conf("rf.conf", &_, domain, path, file_name);

  mvwprintw(backupw, line++, 2, "Starting backup ...");
  sprintf(tmp, "Enter username for %s:", domain);
  mvwprintw(backupw, line++, 2, tmp);
  wrefresh(backupw);

  if (sshs == NULL)
    return -1;

  user = get_string(backupw, line++, 2, 0);

  sprintf(tmp, "Connecting to %s@%s ...", user, domain);
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  ssh_options_set(sshs, SSH_OPTIONS_HOST, domain);
  ssh_options_set(sshs, SSH_OPTIONS_USER, user);

  prev_tmp = strlen(tmp);

  if ((rc = ssh_connect(sshs)) != SSH_OK) {
    ssh_free(sshs);
    sprintf(tmp, " Could not connect.");
    mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
    sprintf(tmp, "Check internet connection or something.");
    mvwprintw(backupw, line, 2, tmp);
    wrefresh(backupw);
    return -1;
  }

  sprintf(tmp, " Connected!");
  mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
  wrefresh(backupw);

  // TODO Authenticate server
  sprintf(tmp, "Enter password: ");
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  password = get_string(backupw, line, 18, 1);

  sprintf(tmp, "Authenticating ...");
  mvwprintw(backupw, ++line, 2, tmp);
  wrefresh(backupw);

  prev_tmp = strlen(tmp);
  if ((rc = ssh_userauth_password(sshs, NULL, password))
      != SSH_AUTH_SUCCESS) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    sprintf(tmp, " Failed.");
    mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
    sprintf(tmp, "Wrong password for user '%s'.", user);
    mvwprintw(backupw, line, 2, tmp);
    wrefresh(backupw);
    return -1;
  }
  sprintf(tmp, " OK");
  mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
  wrefresh(backupw);

  sprintf(tmp, "Opening SCP connection to %s ...", path);
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  if ((scp = ssh_scp_new(sshs, SSH_SCP_WRITE, path)) == NULL) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  if ((rc = ssh_scp_init(scp)) != SSH_OK) {
    // TODO Say so!
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
  wrefresh(backupw);

  // Assuming this file will never be bigger than 8 MiB
  buffer = malloc(0x800000);

  sprintf(tmp, "Reading %s ...", file_name);
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  member_file = fopen(file_name, "rb");
  size = fread(buffer, 1, 0x800000, member_file);
  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
  wrefresh(backupw);

  sprintf(tmp, "Uploading ...");
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  if ((rc = ssh_scp_push_file(scp, file_name, size, 0644)) != SSH_OK) {
    // TODO Say so!
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  if ((rc = ssh_scp_write(scp, buffer, size)) != SSH_OK) {
    // TODO Say so!
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
  wrefresh(backupw);
  mvwprintw(backupw, line, 2, "Backup completed. Press any key.");
  wrefresh(backupw);

  free(buffer);
  ssh_disconnect(sshs);
  ssh_free(sshs);
  free(password);
  free(user);
  return 0;
}

int ssh_fetch_db(WINDOW *main_win) {
  ssh_session sshs = ssh_new();
  ssh_scp scp;
  int rc, size, prev_tmp, line = 1, _;
  char *user, domain[80], *password, path[100], *buffer, tmp[300];
  char file_name[80], *pfn;
  FILE *member_file;

  read_conf("rf.conf", &_, domain, path, file_name);

  pfn = malloc(sizeof(char)*(strlen(file_name) + strlen(path)));

  mvwprintw(main_win, line++, 2, "Fetching database ...");
  sprintf(tmp, "Enter username for %s:", domain);
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);

  if (sshs == NULL)
    return -1;

  user = get_string(main_win, line++, 23 + strlen(domain), 0);

  sprintf(tmp, "Connecting to %s@%s ...", user, domain);
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);
  ssh_options_set(sshs, SSH_OPTIONS_HOST, domain);
  ssh_options_set(sshs, SSH_OPTIONS_USER, user);

  prev_tmp = strlen(tmp);

  if ((rc = ssh_connect(sshs)) != SSH_OK) {
    ssh_free(sshs);
    sprintf(tmp, " Could not connect.");
    mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
    sprintf(tmp, "Check internet connection or something.");
    mvwprintw(main_win, line, 2, tmp);
    wrefresh(main_win);
    return -1;
  }

  sprintf(tmp, " Connected!");
  mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
  wrefresh(main_win);

  // TODO Authenticate server
  sprintf(tmp, "Enter password: ");
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);
  password = get_string(main_win, line, 18, 1);

  sprintf(tmp, "Authenticating ...");
  mvwprintw(main_win, ++line, 2, tmp);
  wrefresh(main_win);

  prev_tmp = strlen(tmp);
  if ((rc = ssh_userauth_password(sshs, NULL, password))
      != SSH_AUTH_SUCCESS) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    sprintf(tmp, " Failed.");
    mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
    sprintf(tmp, "Wrong password for user '%s'.", user);
    mvwprintw(main_win, line, 2, tmp);
    wrefresh(main_win);
    return -1;
  }
  sprintf(tmp, " OK");
  mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
  wrefresh(main_win);
  free(password);
  free(user);

  sprintf(tmp, "Opening SCP connection to %s ...", path);
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);
  sprintf(pfn, "%s%s", path, file_name);
  if ((scp = ssh_scp_new(sshs, SSH_SCP_READ, pfn)) == NULL) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  rc = ssh_scp_init(scp);
  rc = ssh_scp_pull_request(scp);
  //  filename = strdup(ssh_scp_request_get_filename(scp));
  //  mode = ssh_scp_request_get_permissions(scp);
  size = ssh_scp_request_get_size(scp);
  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
  wrefresh(main_win);

  buffer = malloc(size);

  sprintf(tmp, "Downloading %s (%d) ...", file_name, size);
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);
  ssh_scp_accept_request(scp);
  if ((rc = ssh_scp_read(scp, buffer, size)) != SSH_SCP_REQUEST_EOF) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
  wrefresh(main_win);

  sprintf(tmp, "Writing ...");
  mvwprintw(main_win, line, 2, tmp);
  wrefresh(main_win);
  member_file = fopen(file_name, "wb");
  fwrite(buffer, 1, size, member_file);
  free(buffer);
  free(pfn);
  fclose(member_file);

  if ((rc = ssh_scp_pull_request(scp)) != SSH_SCP_REQUEST_EOF) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }

  prev_tmp = strlen(tmp);
  sprintf(tmp, " Done!");
  mvwprintw(main_win, line++, 2 + prev_tmp, tmp);
  wrefresh(main_win);
  mvwprintw(main_win, line, 2, "Fetch completed. Press any key.");
  wrefresh(main_win);

  ssh_disconnect(sshs);
  ssh_free(sshs);
  getch();
  return 0;
}

static int search_callback(void *vcurr, int argc,
                    char **member, char **colname) {
  int x, y;
  callback_container *cbc = vcurr;

  WINDOW *padw = cbc->padw;
  int *curr = cbc->curr;
  int *curr_line = cbc->curr_line;
  int *visible_members = cbc->visible_members;
  int *delete_rowid = cbc->delete_rowid;
  
  getmaxyx(padw, y, x);
  if (*curr == *curr_line)
    wattron(padw, A_REVERSE | A_BOLD);
  mvwprintw(padw, *curr, 2, " %s %s ", member[0], member[1]);
  long ts = strtol(member[2], NULL, 10);
  char *ts_36 = base36(ts);
  mvwprintw(padw, *curr, x - 26, " %s ", asctime(localtime(&ts)));
  wattron(padw, A_BOLD);
  mvwprintw(padw, *curr, x - 34, " %s ", ts_36);
  wattroff(padw, A_BOLD);
  if ((*curr)++ == *curr_line) {
    wattroff(padw, A_REVERSE | A_BOLD);
    *delete_rowid = atoi(member[3]);
  }
  (*visible_members)++;
  free(ts_36);
  return 0;
}

int search(sqlite3 *db, WINDOW *main_win, WINDOW *padw,
           char *needle, long period_begin, long period_end,
           int *curr_line, int *visible_members, int *delete_rowid,
           int curr_scroll) {
  *visible_members = 0;
  char sqls[500];
  int y, x, *c;
  c = malloc(sizeof(int));
  *c = 0;

  callback_container curr = {padw, c, curr_line,
                             visible_members, delete_rowid};

  getmaxyx(main_win, y, x);
  werase(padw);
  sprintf(sqls, "SELECT first_name, last_name, timestamp, \
rowid FROM members WHERE (last_name LIKE '%%%s%%' OR first_name \
LIKE '%%%s%%') AND timestamp > %ld ORDER BY timestamp DESC",
needle, needle, period_begin);
  sqlite3_exec(db, sqls, &search_callback, &curr, 0);
  prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
  free(c);
  return 0;
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

static int stats_callback(void *n, int argc,
                          char **member, char **colnames) {
  (*((int *) n))++;
  return 0;
}

void stats(sqlite3 *db, WINDOW *main_win, int bg, long semstart,
           int price, int w) {
  // TODO This semester so far compared to avg. spring/autumn semester
  int x, y, ch, last24 = 0, thissem = 0;
  int lifetimers = 0, new_lifetimers = 0;
  char sqls[500];
  char tmp[500];

  // Count last 24h
  sprintf(sqls, "SELECT * FROM members WHERE timestamp > %ld",
          time(NULL)-86400);
  sqlite3_exec(db, sqls, &stats_callback, &last24, 0);

  // Count this semester
  sprintf(sqls, "SELECT * FROM members WHERE timestamp > %ld",
          semstart);
  sqlite3_exec(db, sqls, &stats_callback, &thissem, 0);

  // Count lifetimers
  sqlite3_exec(db, "SELECT * FROM members WHERE lifetime == 1",
               &stats_callback, &lifetimers, 0);

  // Count new lifetimers
  sprintf(sqls, "SELECT * FROM members WHERE timestamp > %ld \
AND lifetime == 1", semstart);
  sqlite3_exec(db, sqls, &stats_callback, &new_lifetimers, 0);

  if (bg == IN_BACKGROUND) {
    mvprintw(1, COLS - 33, "NOK: %5d TDY: %5d TTL: %5d",
             last24 * price, last24, thissem);
    return;
  }

  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);

  int t = 10;

  sprintf(tmp, "LAST 24 HOURS");
  mvwprintw(main_win, t, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "New members:   %8d", last24);
  mvwprintw(main_win, t + 1, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "Revenue (NOK): %8d", last24 * price);
  mvwprintw(main_win, t + 2, (x - strlen(tmp)) >> 1, tmp);

  sprintf(tmp, "THIS SEMESTER");
  mvwprintw(main_win, t + 5, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "Lifetimers:    %8d", lifetimers);
  mvwprintw(main_win, t + 6, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "New lifetimers:%8d", new_lifetimers);
  mvwprintw(main_win, t + 7, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "New members:   %8d", thissem);
  mvwprintw(main_win, t + 8, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "Total members: %8d", thissem + lifetimers);
  mvwprintw(main_win, t + 9, (x - strlen(tmp)) >> 1, tmp);
  sprintf(tmp, "Revenue (NOK): %8d", thissem * price
          + new_lifetimers * price * 10);
  mvwprintw(main_win, t + 10, (x - strlen(tmp)) >> 1, tmp);

  wrefresh(main_win);
  if (w == NOWAIT)
    return;
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

void debug(char *msg) {
  mvprintw(0, 0, msg);
  update_panels();
  doupdate();
}

void register_member(sqlite3 *db, WINDOW *main_win, char *f_name,
                     char *l_name, bool lifetime, long ts,
                     long semstart, int price) {
  char sqls[500];
  char *errmsg = 0;
  sprintf(sqls, "INSERT INTO members (first_name, last_name, \
lifetime, timestamp, paid) VALUES ('%s', '%s', %d, %ld, %d)", f_name, l_name,
          (int) lifetime, ts, price);
  sqlite3_exec(db, sqls, 0, 0, &errmsg);
  stats(db, main_win, IN_BACKGROUND, semstart, price, NOWAIT);
}

void read_conf(char *conf_name, int *price, char *domain,
               char *path, char *file_name) {
  FILE *fp = fopen(conf_name, "r");
  fscanf(fp, "%d %s %s %s", price, domain, path, file_name);
  fclose(fp);
}

char *strtok2(char *line, char tok) {
  char *tmp = strdup(line);
  int i = 0;
  while (*line != tok && *line != 0) {
    line++;
    i++;
  }
  if (*line == 0)
    return NULL;
  line++;
  return strndup(tmp, i);
}

int *sem2my(char *sem) {
  int *r = malloc(sizeof(int) * 2);
  char s = *sem++;
  r[0] = s == 'h' && s == 'H' ? 6 : 0;
  while (!isdigit(*sem))
    sem++;
  int year = atoi(sem);
  r[1] = year < 70 ? 2000 + year : 1900 + year;
  return r;
}

int csv2reg(char *line) {
  char *comment, *name, *sem, *issued_by;
  int num;
  long ts = 0;
  char ln[500];

  num = atoi(strtok2(line, ','));
  comment = strtok2(line, ',');
  name = strtok2(line, ',');
  sem = strtok2(line, ',');
  issued_by = strtok2(line, ',');

  if (*sem == 0) {
    struct tm *now = gmtime(0);
    int *my_sem = sem2my(sem);
    now->tm_year = my_sem[1];
    now->tm_mon = my_sem[0];
    ts = mktime(now);
  }

  sprintf(ln, "(%s)", comment);

  //  register_member(name, ln, true, ts, price);

  free(line);
  return 0;
}

int read_buffer(char *buffer) {
  char *line = NULL;
  int i = 0;
  line = strtok(buffer, "\n");
  while (line != NULL) {
    csv2reg(line);
    line = strtok(NULL, "\n");
    i++;
  }
  return i;
}

/*int get_lifetimers() {
  ssh_session sshs = ssh_new();
  ssh_scp scp;
  int rc, size, num_lt_members;
  char *password, *buffer;

  if (sshs == NULL)
    return -1;

  // TODO Read host, user from config file
  ssh_options_set(sshs, SSH_OPTIONS_HOST, "login.ifi.uio.no");
  ssh_options_set(sshs, SSH_OPTIONS_USER, "rf");
  rc = ssh_connect(sshs);
  if (rc != SSH_OK) {
    ssh_free(sshs);
    return -1;
  }

  // TODO Authenticate server
  // TODO Read password from user
  //  password = get_string();
  rc = ssh_userauth_password(sshs, NULL, password);
  if (rc != SSH_AUTH_SUCCESS) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }

  // TODO Read path from config file
  scp = ssh_scp_new(sshs, SSH_SCP_READ, "Sekretos/.../livstid.csv");
  if (scp == NULL) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  rc = ssh_scp_init(scp);
  rc = ssh_scp_pull_request(scp);
  size = ssh_scp_request_get_size(scp);
  buffer = malloc(size);
  //  filename = strdup(ssh_scp_request_get_filename(scp));
  //  mode = ssh_scp_request_get_permissions(scp);

  ssh_scp_accept_request(scp);
  rc = ssh_scp_read(scp, buffer, size);

  // TODO Write new 'parseline' to comply with format
  num_lt_members = read_buffer(buffer);

  free(buffer);
  rc = ssh_scp_pull_request(scp);
  if (rc != SSH_SCP_REQUEST_EOF) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  ssh_disconnect(sshs);
  ssh_free(sshs);
  return num_lt_members;
}*/
