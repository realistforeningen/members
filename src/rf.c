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

void destroy_win(WINDOW *local_win);
WINDOW *newwin(int height, int width, int starty, int startx);

bool delete(int dl, int curr_scroll);
bool dialog_sure();
char *strstrip(char *str);
int csv2reg(char *line);
int get_lifetimers();
int search(char *needle, int hl, long period_begin, long period_end,
           int *curr_line, int *visible_members, int *delete_rowid,
           int curr_scroll);
int ssh_backup();
int *sem2my(char *sem);
void backup();
void debug(char *msg);
void home(int w);
void members(int bg, long period_begin, long period_end,
             int *curr_scroll, const int PRICE);
void menu(long semstart, const int PRICE);
void printmenu(WINDOW *mw, char **menu_s, int ML, int active);
void refresh_bg(int prev_y, int curr_scroll, long semstart,
                const int PRICE);
void reg(int curr_scroll, long semstart, const int PRICE);
void register_member(char *f_name, char *l_name, bool lifetime,
                     long ts, long semstart, const int PRICE);
void stats(int bg, long semstart, const int PRICE, int w);

// GLOBALS
sqlite3 *db;
PANEL *panels[4];
WINDOW *main_win, *padw;

int main() {
  const int PRICE = 50;
  long semstart;
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
  wrefresh(main_win);
  ESCDELAY = 0;
  const char *RF = "R E A L I S T F O R E N I N G E N";
  mvprintw(0, (COLS - strlen(RF)) >> 1, RF);
  mvprintw(1, 2, "Menu");

  // Define start of semester
  time_t ts_now = time(NULL);
  struct tm *now = gmtime(&ts_now);
  now->tm_hour = 0;
  now->tm_min = 0;
  now->tm_sec = 0;
  now->tm_mday = 1;
  now->tm_mon = now->tm_mon < 6 ? 0 : 6;
  semstart = (long) mktime(now);

  // Open database
  sqlite3_open("members.db", &db);
  char *errmsg;
  char *sql = "CREATE TABLE IF NOT EXISTS members\
 (first_name NCHAR(50) NOT NULL, last_name NCHAR(50) NOT NULL,\
 lifetime TINYINT, timestamp BIGINT NOT NULL)";
  sqlite3_exec(db, sql, NULL, NULL, &errmsg);
  //  debug(errmsg);
  stats(IN_BACKGROUND, semstart, PRICE, WAIT);

  // Start main loop
  home(WAIT);
  menu(semstart, PRICE);

  endwin();
  return 0;
}

void menu(long semstart, const int PRICE) {
  int active_y = 0, prev_y = 0, i, ch;
  char **menu_s;
  WINDOW *menu_win;
  const int MENU_LEN = 6;
  int curr_scroll = 0, x, y;

  panels[0] = new_panel(main_win);
  //  panels[0] = new_panel(padw);

  menu_s = malloc(sizeof(char*) * MENU_LEN);
  for (i = 0; i < MENU_LEN; i++)
    menu_s[i] = malloc(sizeof(char) * 15);
  menu_s[0] = " Home     ";
  menu_s[1] = " Register ";
  menu_s[2] = " Members  ";
  menu_s[3] = " Stats    ";
  menu_s[4] = " Backup   ";
  menu_s[5] = " Exit     ";

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
      if (dialog_sure())
        return;
      prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
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
        home(WAIT);
        break;
      case 1:
        reg(curr_scroll, semstart, PRICE);
        break;
      case 2:
        members(IN_FOREGROUND, semstart, (long) time(NULL),
                &curr_scroll, PRICE);
        break;
      case 3:
        stats(IN_FOREGROUND, semstart, PRICE, WAIT);
        break;
      case 4:
        backup();
        refresh_bg(prev_y, curr_scroll, semstart, PRICE);
        break;
      case 5:
        if (dialog_sure())
          return;
        refresh_bg(prev_y, curr_scroll, semstart, PRICE);
      }
      prev_y = active_y != 1 && active_y != 4
        && active_y != 5 ? active_y : prev_y;
    }
  }
  for (i = 0; i < MENU_LEN; i++)
    free(menu_s[i]);
  free(menu_s);
}

void refresh_bg(int prev_y, int curr_scroll, long semstart,
                const int PRICE) {
  int x, y;
  getmaxyx(main_win, y, x);
  switch (prev_y) {
  case 0:
    home(NOWAIT);
    break;
  case 1:
  case 2:
    prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
    break;
  case 3:
    stats(IN_FOREGROUND, semstart, PRICE, NOWAIT);
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

void home(int w) {
  // TODO dancing bear
  int i;
  char *bear[10];
  int x, y, ch;

  getmaxyx(main_win, y, x);
  werase(main_win);
  box(main_win, 0, 0);

  bear[0] = "      _____        _";
  bear[1] = "    d8888888b.   d888b   ,db";
  bear[2] = "   d888888888888888888888888.*";
  bear[3] = "  888888 88888888888 88 8888888o";
  bear[4] = "  8888888 888888888 8888 8888`~~";
  bear[5] = "  8888888 888888888 88888";
  bear[6] = "   888888 8888888888 8888";
  bear[7] = "  ## 88888  88888 ##  8888";
  bear[8] = " #### 88888      ###   8888";
  bear[9] = "###,,, 888,,,    ##,,,  88,,,";

  for (i = 0; i < 10; i++)
    mvwprintw(main_win, (y >> 1) - 7 + i,
              (x - strlen(bear[3])) >> 1, bear[i]);

  char *s = "Dette er Realistforeningens medlemsliste.";
  mvwprintw(main_win, (y >> 1) - 6 + i, (x - strlen(s)) >> 1, s);
  wrefresh(main_win);
  if (NOWAIT)
    return;
  for (;;) {
    switch (ch = getch()) {
    case 27:
      return;
    }
  }
}

void reg(int curr_scroll, long semstart, const int PRICE) {
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
  prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
  post_form(rf_form);
  mvwprintw(dwin, 1, 1, "  Fornavn:");
  mvwprintw(dwin, 3, 1, "Etternavn:");
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
      if (!(*f_name & *l_name))
        break; // Don't allow empty names
      register_member(f_name, l_name, false, time(NULL),
                      semstart, PRICE);
      members(IN_BACKGROUND, semstart, (long) time(NULL), &curr_scroll,
              PRICE);
      prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
      box(formw, 0, 0);
      form_driver(rf_form, REQ_CLR_FIELD);
      form_driver(rf_form, REQ_NEXT_FIELD);
      form_driver(rf_form, REQ_CLR_FIELD);
      form_driver(rf_form, REQ_NEXT_FIELD);
      mvwprintw(dwin, 1, 1, "  Fornavn:");
      mvwprintw(dwin, 3, 1, "Etternavn:");
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
      return;
    default:
      form_driver(rf_form, ch);
      break;
    }
    wrefresh(formw);
  }
}

void members(int bg, long period_begin, long period_end, int *curr_scroll,
             const int PRICE) {
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
  search("", 0, period_begin, period_end, &curr_line, &visible_members,
         &delete_rowid, *curr_scroll);
  bool search_mode = false;
  char *needle_buf = "", *send_s;
  bool btm = false; // Hack to make scrolling work
  prefresh(padw, *curr_scroll, 1, 3, 1, y, x-2);
  if (bg)
    return;
  for (;;) {
    switch (ch = getch()) {
    case KEY_DOWN:
      if (curr_line == visible_members - 1) {
        btm = false;
        break;}
      search(needle_buf, ++curr_line, period_begin, (long) time(NULL),
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
      search(needle_buf, --curr_line, period_begin, (long) time(NULL),
             &curr_line, &visible_members, &delete_rowid, *curr_scroll);
      if (curr_line < *curr_scroll)
        prefresh(padw, --(*curr_scroll), 1, 3, 1, y, x-2);
      move(1, 27 + needle_idx);
      break;
    case KEY_BACKSPACE:
      if (search_mode && needle_idx > 0) {
        needle_buf[--needle_idx] = '\0';
        send_s = (char *) malloc(needle_idx + 1);
        strncpy(send_s, needle_buf, needle_idx+1);
        search(send_s, curr_line = 0, period_begin, (long) time(NULL),
               &curr_line, &visible_members, &delete_rowid, *curr_scroll);
        free(send_s);
        mvprintw(1, 26, "                         ");
        mvprintw(1, 27, "%s", needle_buf);
      }
      break;
    case KEY_DC: // Delete
      if (!delete(delete_rowid, *curr_scroll))
        break;
      stats(IN_BACKGROUND, period_begin, PRICE, WAIT);
      curr_line = 0;
      search(needle_buf, 0, period_begin, (long) time(NULL), &curr_line,
             &visible_members, &delete_rowid, *curr_scroll);
      break;
    case 47:
      if (!search_mode) {
        curr_line = 0;
        *curr_scroll = 0;
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
        search("", 0, period_begin, (long) time(NULL), &curr_line,
               &visible_members, &delete_rowid, *curr_scroll);
        free(needle_buf);
        mvprintw(1, 19, "                                ");
        prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
        break;
      }
      curr_line = 0;
      prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
      return;
    default:
      if (search_mode && needle_idx < 32) {
        werase(padw);
        needle_buf[needle_idx++] = (char) ch;
        needle_buf[needle_idx] = '\0';
        send_s = (char *) malloc(needle_idx + 1);
        strncpy(send_s, needle_buf, needle_idx + 1);
        search(send_s, 0, period_begin, (long) time(NULL), &curr_line,
               &visible_members, &delete_rowid, *curr_scroll);
        free(send_s);
        prefresh(padw, *curr_scroll, 1, 3, 1, y, x - 2);
        mvprintw(1, 26, " %s", needle_buf);
      }
    }
  }
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

bool dialog_sure() {
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
      return true;
      }
    case 110: // n
      hide_panel(panels[3]);
      update_panels();
      doupdate();
      return false;
    }
  }
}

bool delete(int dl, int curr_scroll) {
  int x, y;
  getmaxyx(main_win, y, x);  
  if (!dialog_sure()) {
    prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
    return false;
  }
  char sqls[50];
  sprintf(sqls, "DELETE FROM members WHERE rowid == %d AND\
 lifetime == 0", dl);
  sqlite3_exec(db, sqls, 0, 0, 0);
  prefresh(padw, curr_scroll, 1, 3, 1, y, x-2);
  return true;
}

void backup() {
  int x, y, ch, h = 15, wi = 70;
  WINDOW *backupw;
  hide_panel(panels[1]);
  getmaxyx(main_win, y, x);

  backupw = newwin(h + 3, wi + 4, (y - h) >> 1, (x - wi) >> 1);
  panels[3] = new_panel(backupw);
  update_panels();
  doupdate();
  box(backupw, 0, 0);

  ssh_backup(backupw);
  ch = getch(); // Wait for any key

  hide_panel(panels[3]);
  update_panels();
  doupdate();
  return;
}

char *get_password(WINDOW *w, int y, int x) {
  char *pass, *rf;
  int ch, i = 0;
  pass = malloc(sizeof(char)*30);
  curs_set(1);
  for (;;) {
    switch (ch = getch()) {
    case KEY_BACKSPACE:
      if (i)
        i--;
      mvwprintw(w, y, x + i, " ");
      wmove(w, y, x + i);
      wrefresh(w);
      break;
    case 10:
      curs_set(0);
      pass[i] = '\0';
      return pass;
    default:
      if (i > 30)
        break;
      pass[i] = ch;
      rf = i % 2 ? "F" : "R";
      mvwprintw(w, y, x + i++, rf);
      wrefresh(w);
      break;
    }
  }
}

int ssh_backup(WINDOW *backupw) {
  ssh_session sshs = ssh_new();
  ssh_scp scp;
  int rc, size, prev_tmp, line = 1;
  char *user, *domain, *password, *path, *buffer, tmp[300];
  char *file_name;
  FILE *member_file;

  // TODO Read this from rf.conf
  user = "rf";
  domain = "login.ifi.uio.no";
  path = "Kjellerstyret/medlemsliste/";
  file_name = "members.db";

  mvwprintw(backupw, line++, 2, "Starting backup ...");
  wrefresh(backupw);

  if (sshs == NULL)
    return -1;

  sprintf(tmp, "Connecting to %s@%s ...", user, domain);
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  ssh_options_set(sshs, SSH_OPTIONS_HOST, domain);
  ssh_options_set(sshs, SSH_OPTIONS_USER, user);
  rc = ssh_connect(sshs);

  prev_tmp = strlen(tmp);

  if (rc != SSH_OK) {
    ssh_free(sshs);
    sprintf(tmp, " Could not connect.");
    mvwprintw(backupw, line++, 2 + prev_tmp, tmp);
    sprintf(tmp, "Check internet connection.");
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
  password = get_password(backupw, line, 18);

  sprintf(tmp, "Authenticating ...");
  mvwprintw(backupw, ++line, 2, tmp);
  wrefresh(backupw);

  prev_tmp = strlen(tmp);
  rc = ssh_userauth_password(sshs, NULL, password);
  if (rc != SSH_AUTH_SUCCESS) {
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
  free(password);

  sprintf(tmp, "Opening SCP connection to %s ...", path);
  mvwprintw(backupw, line, 2, tmp);
  wrefresh(backupw);
  scp = ssh_scp_new(sshs, SSH_SCP_WRITE, path);
  if (scp == NULL) {
    ssh_disconnect(sshs);
    ssh_free(sshs);
    return -1;
  }
  rc = ssh_scp_init(scp);
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
  rc = ssh_scp_push_file(scp, file_name, size, 0644);
  rc = ssh_scp_write(scp, buffer, size);
  if (rc != SSH_OK) {
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
  return 0;
}

int search_callback(void *vcurr, int argc,
                           char **member, char **colname) {
  int x, y;
  int **vicurr = vcurr;
  int *curr = vicurr[0];
  int *curr_line = vicurr[1];
  int *visible_members = vicurr[2];
  int *delete_rowid = vicurr[3];
  
  getmaxyx(main_win, y, x);
  if (*curr == *curr_line)
    wattron(padw, A_REVERSE);
  mvwprintw(padw, *curr, 2, "%s %s", member[0], member[1]);
  long ts = strtol(member[2], NULL, 10);
  mvwprintw(padw, *curr, x - 26, "%s", asctime(localtime(&ts)));
  if ((*curr)++ == *curr_line) {
    wattroff(padw, A_REVERSE);
    *delete_rowid = atoi(member[3]);
  }
  (*visible_members)++;
  return 0;
}

int search(char *needle, int hl, long period_begin, long period_end,
           int *curr_line, int *visible_members, int *delete_rowid,
           int curr_scroll) {
  *visible_members = 0;
  char sqls[500];
  int **curr, y, x, *c;
  curr = malloc(sizeof(int *)*4);
  c = malloc(sizeof(int));
  *c = 0;
  curr[0] = c;
  curr[1] = curr_line;
  curr[2] = visible_members;
  curr[3] = delete_rowid;

  getmaxyx(main_win, y, x);
  werase(padw);
  sprintf(sqls, "SELECT first_name, last_name, timestamp, \
rowid FROM members WHERE (last_name LIKE '%%%s%%' OR first_name \
LIKE '%%%s%%') AND timestamp > %ld ORDER BY timestamp DESC",
needle, needle, period_begin);
  sqlite3_exec(db, sqls, &search_callback, curr, 0);
  prefresh(padw, curr_scroll, 1, 3, 1, y, x - 2);
  free(curr);
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
  int *m = n;
  (*m)++;
  return 0;
}

void stats(int bg, long semstart, const int PRICE, int w) {
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
             last24 * PRICE, last24, thissem);
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
  sprintf(tmp, "Revenue (NOK): %8d", last24 * PRICE);
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
  sprintf(tmp, "Revenue (NOK): %8d", thissem * PRICE
          + new_lifetimers * PRICE * 10);
  mvwprintw(main_win, t + 10, (x - strlen(tmp)) >> 1, tmp);

  wrefresh(main_win);
  if (NOWAIT)
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

void register_member(char *f_name, char *l_name, bool lifetime, long ts,
                     long semstart, const int PRICE) {
  char sqls[500];
  char *errmsg = 0;
  sprintf(sqls, "INSERT INTO members (first_name, last_name, \
lifetime, timestamp) VALUES ('%s', '%s', %d, %ld)", f_name, l_name,
          (int) lifetime, ts);
  sqlite3_exec(db, sqls, 0, 0, &errmsg);
  stats(IN_BACKGROUND, semstart, PRICE, NOWAIT);
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
    ts = (long) mktime(now);
  }

  sprintf(ln, "(%s)", comment);

  //  register_member(name, ln, true, ts, PRICE);

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
  //  password = get_password();
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
