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
