#include <assert.h>
#include <locale.h>
#include <ncurses.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "tui.h"
#include "utils.h"

static unsigned _upd_interval;

enum {
	PAIR_DEF = 1,
	PAIR_GREEN_DEF = 2,
	PAIR_RED_DEF = 3
};

static void print_banner(WINDOW *win);
static void iq_draw_quadrants(WINDOW *win);
static void windows_init(int rows, int col);

struct {
	WINDOW *banner_top;
	WINDOW *iq;
	WINDOW *pll, *filein, *dataout;
	WINDOW *infowin;
} tui;

void
tui_init(unsigned upd_interval)
{
	int rows, cols;
	setlocale(LC_ALL, "");

	initscr();
	noecho();
	cbreak();
	curs_set(0);

	use_default_colors();
	start_color();
	init_pair(PAIR_DEF, -1, -1);
	init_pair(PAIR_RED_DEF, COLOR_RED, -1);
	init_pair(PAIR_GREEN_DEF, COLOR_GREEN, -1);

	getmaxyx(stdscr, rows, cols);
	_upd_interval = upd_interval;

	windows_init(rows, cols);
	print_banner(tui.banner_top);
	tui_update_pll(0, 0);
	iq_draw_quadrants(tui.iq);
}

void
tui_handle_resize()
{
	int nr, nc;
	int iq_size, cur_row;

	/* Re-initialize ncurses with the correct dimensions */
	werase(stdscr);
	endwin();

	refresh();
	getmaxyx(stdscr, nr, nc);

	iq_size = MIN(CONSTELL_MAX, nr);
	cur_row = 0;

	wresize(tui.banner_top, 1, nc);
	mvwin(tui.banner_top, 0, 0);
	wresize(tui.iq, iq_size/2, iq_size);
	mvwin(tui.iq, 2, 0);
	wresize(tui.pll, iq_size/6, nc - iq_size);
	mvwin(tui.pll, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	wresize(tui.filein, iq_size/6, nc - iq_size);
	mvwin(tui.filein, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	wresize(tui.dataout, iq_size/6, nc - iq_size);
	mvwin(tui.dataout, 2+cur_row, iq_size+2);
	wresize(tui.infowin, MIN(nr - iq_size, 10), nc);
	mvwin(tui.infowin, 2+iq_size/2+2, 0);

	print_banner(tui.banner_top);
	iq_draw_quadrants(tui.iq);
	wrefresh(tui.iq);
}

int
tui_process_input()
{
	int ch;
	ch = wgetch(tui.infowin);

	switch(ch) {
	case KEY_RESIZE:
		tui_handle_resize();
		break;
	case 'q':
		return 1;
		break;
	default:
		break;
	}
	wrefresh(tui.infowin);
	return 0;
}

int
tui_print_info(const char *msg, ...)
{
	time_t t;
	va_list ap;
	struct tm* tm;
	char timestr[] = "HH:MM:SS";

	assert(tui.infowin);

	t = time(NULL);
	tm = localtime(&t);
	strftime(timestr, sizeof(timestr), "%T", tm);
	wprintw(tui.infowin, "(%s) ", timestr);
	va_start(ap, msg);
	vwprintw(tui.infowin, msg, ap);
	va_end(ap);

	return 0;
}

void
tui_update_pll(float freq, int islocked)
{
	werase(tui.pll);
	wmove(tui.pll, 0, 0);
	wattrset(tui.pll, A_BOLD);
	wprintw(tui.pll, "PLL info\n");
	wattroff(tui.pll, A_BOLD);
	wprintw(tui.pll, "Carrier Freq     Status\n");
	wprintw(tui.pll, "%+7.1f Hz       ", freq);
	if (islocked) {
		wattrset(tui.pll, COLOR_PAIR(PAIR_GREEN_DEF));
		wprintw(tui.pll, "%s", "Locked");
	} else {
		wattrset(tui.pll, COLOR_PAIR(PAIR_RED_DEF));
		wprintw(tui.pll, "%s", "Acquiring...");
	}
	wattrset(tui.pll, COLOR_PAIR(PAIR_DEF));
	wrefresh(tui.pll);
}

void
tui_draw_constellation(char *dots, unsigned count)
{
	char x, y;
	int nr, nc;
	unsigned i;
	int prev;

	getmaxyx(tui.iq, nr, nc);

	werase(tui.iq);
	for (i=0; i<count; i++) {
		x = dots[i++]*nc/255;
		y = dots[i]*nr/255;

		prev = mvwinch(tui.iq, y+nr/2, x+nc/2);
		switch(prev) {
		case '.':
			waddch(tui.iq, '+');
			break;
		case '+':
			waddch(tui.iq, '#');
			break;
		default:
			waddch(tui.iq, '.');
			break;
		}
	}
	iq_draw_quadrants(tui.iq);
}

/* Update the input file info */
void
tui_update_file_in(float perc)
{
	werase(tui.filein);

	wmove(tui.filein, 0, 0);
	wattrset(tui.filein, A_BOLD);
	wprintw(tui.filein, "Data in\n");
	wattroff(tui.filein, A_BOLD);
	wprintw(tui.filein, "%.1f%%", perc);

	wrefresh(tui.filein);
}

/* Update the data out info */
void
tui_update_data_out(unsigned nbytes)
{
	char humansize[8];
	humanize(nbytes, humansize);

	werase(tui.dataout);
	wattrset(tui.dataout, A_BOLD);
	mvwprintw(tui.dataout, 0, 0, "Data out\n");
	wattroff(tui.dataout, A_BOLD);
	wprintw(tui.dataout, "%sB", humansize);
	wrefresh(tui.dataout);
}

int
tui_wait_for_user_input()
{
	int ret;

	wtimeout(tui.infowin, -1);
	ret = wgetch(tui.infowin);
	wtimeout(tui.infowin, _upd_interval);

	return ret;
}

/* Cleanly deinit ncurses */
void
tui_deinit()
{
	endwin();
}


/* Static functions {{{ */
void
print_banner(WINDOW *win)
{
	mvwprintw(win, 0, 0, "\t~ Meteor M2 LRPT Demodulator ~");
	wrefresh(win);
}

void
windows_init(int rows, int cols)
{
	int iq_size;
	int cur_row;

	iq_size = MIN(CONSTELL_MAX, cols);
	cur_row = 0;

	tui.banner_top = newwin(1, cols, 0, 0);
	tui.iq = newwin(iq_size/2, iq_size, 2, 0);
	tui.pll = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	tui.filein = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	tui.dataout = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	tui.infowin = newwin(MIN(rows - iq_size/2, 10), cols, 2+iq_size/2+2, 0);

	scrollok(tui.infowin, TRUE);
	wtimeout(tui.infowin, _upd_interval);
}

void
iq_draw_quadrants(WINDOW *win)
{
	int i, nr, nc;

	getmaxyx(win, nr, nc);
	for (i=0; i<nr; i++) {
		mvwaddch(win, i, nc/2, '|');
	}
	for (i=0; i<nc; i++) {
		mvwaddch(win, nr/2, i, '-');
	}
	mvwaddch(win, nr/2, nc/2, '+');
	wrefresh(win);
}
/*}}}*/
