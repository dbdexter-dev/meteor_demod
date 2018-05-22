#include <assert.h>
#include <locale.h>
#include <ncurses.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "tui.h"
#include "utils.h"

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
tui_init()
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

	iq_size = MIN(128, nr);
	cur_row = 0;

	wresize(tui.banner_top, 1, nc);
	mvwin(tui.banner_top, 0, 0);
	wresize(tui.iq, iq_size, iq_size);
	mvwin(tui.iq, 1, 0);
	wresize(tui.pll, iq_size/3, nc - iq_size);
	mvwin(tui.pll, 1+cur_row, iq_size);
	cur_row += iq_size/3;
	wresize(tui.filein, iq_size/3, nc - iq_size);
	mvwin(tui.filein, 1+cur_row, iq_size);
	cur_row += iq_size/3;
	wresize(tui.dataout, iq_size/3, nc - iq_size);
	mvwin(tui.dataout, 1+cur_row, iq_size);
	wresize(tui.infowin, nr - iq_size, nc);
	mvwin(tui.infowin, iq_size, 0);

	print_banner(tui.banner_top);
	iq_draw_quadrants(tui.iq);
	wrefresh(tui.iq);
}

void
tui_process_input()
{
	int ch;
	ch = wgetch(tui.infowin);

	switch(ch) {
	case KEY_RESIZE:
		tui_handle_resize();
		break;
	default:
		break;
	}
	wrefresh(tui.infowin);
}

void
tui_print_info(char *msg, ...)
{
	time_t t;
	va_list ap;
	struct tm* tm;
	char timestr[] = "HH:MM:SS";

	assert(tui.infowin);

	t = time(NULL);
	tm = localtime(&t);
	strftime(timestr, sizeof(timestr), "%T", tm);
	wprintw(tui.infowin, "(%s) ", timestr, msg);
	va_start(ap, msg);
	vwprintw(tui.infowin, msg, ap);
	va_end(ap);
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
			waddch(tui.iq, '*');
			break;
		case '*':
			waddch(tui.iq, '+');
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
	wprintw(tui.filein, "Input file\n");
	wattroff(tui.filein, A_BOLD);
	wprintw(tui.filein, "%.1f%% in", perc);

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

/* Cleanly deinit ncurses */
void
tui_deinit()
{
	tui_print_info("Decoding completed. Will close in 5 seconds...\n");
	wrefresh(tui.infowin);
	sleep(5);
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

	iq_size = MIN(31, cols);
	cur_row = 0;

	tui.banner_top = newwin(1, cols, 0, 0);
	tui.iq = newwin(iq_size/2, iq_size, 2, 0);
	tui.pll = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	tui.filein = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	cur_row += iq_size/6;
	tui.dataout = newwin(iq_size/6, cols-iq_size-2, 2+cur_row, iq_size+2);
	tui.infowin = newwin(MIN(rows - iq_size, 10), cols, 2+iq_size/2+2, 0);

	scrollok(tui.infowin, TRUE);
	wtimeout(tui.infowin, UPD_INTERVAL);
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
