#ifndef _METEOR_TUI_H
#define _METEOR_TUI_H

#include <stdint.h>

#define UPD_INTERVAL 50
#define CONSTELL_MAX 31

void tui_init();
void tui_deinit();
void tui_handle_resize();

int  tui_process_input();

int  tui_print_info(const char *msg, ...);
void tui_update_pll(float freq, int islocked);
void tui_draw_constellation(const int8_t *dots, unsigned count);
void tui_update_file_in(unsigned rate, unsigned done, unsigned duration);
void tui_update_data_out(unsigned nbytes);
int  tui_wait_for_user_input();

#endif
