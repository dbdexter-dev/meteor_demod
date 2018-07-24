/**
 * Auxiliary functions abstracting the ncurses text user interface
 */
#ifndef METEOR_TUI_H
#define METEOR_TUI_H

#include <stdint.h>

#define CONSTELL_MAX 31

void tui_init(unsigned upd_interval);
void tui_deinit(void);
void tui_handle_resize(void);

int  tui_process_input(void);

int  tui_print_info(const char *msg, ...);
void tui_update_pll(float freq, int islocked, float gain);
void tui_draw_constellation(const int8_t *dots, unsigned count);
void tui_update_file_in(unsigned rate, uint64_t done, uint64_t duration);
void tui_update_data_out(unsigned nbytes);
int  tui_wait_for_user_input(void);

#endif
