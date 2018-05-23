#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "demod.h"
#include "options.h"
#include "tui.h"
#include "utils.h"
#include "wavfile.h"

/* Default values */
#define SYM_RATE 72000
#define INTERP_FACTOR 4

int
main(int argc, char *argv[])
{
	int c;
	Sample *raw_samp;
	Demod *demod;

	/* Command line changeable parameters {{{*/
	int net_port;
	int symbol_rate;
	int wait_on_exit;
	int upd_interval;
	float costas_bw;
	unsigned interp_factor;
	char *out_fname;
	/*}}}*/
	/* Argument handling {{{ */

	/* Initialize the parameters that can be overridden with command-line args */
	optind = 0;
	wait_on_exit = 0;
	upd_interval = UPD_INTERVAL;
	net_port = -1;
	symbol_rate = SYM_RATE;
	costas_bw = COSTAS_BW;
	out_fname = NULL;
	interp_factor = INTERP_FACTOR;

	if (argc < 2) {
		splash();
		usage(argv[0]);
	}

	/* Parse command line args */
	while ((c = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != -1) {
		switch (c) {
		case 'b':
			costas_bw = atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			break;
		case 'n':
			if (net_port < 0) {
				net_port = 0;
			}
			break;
		case 'o':
			out_fname = optarg;
			break;
		case 'p':
			net_port = atoi(optarg);
			break;
		case 'r':
			symbol_rate = atoi(optarg);
			break;
		case 'R':
			upd_interval = atoi(optarg);
			break;
		case 's':
			interp_factor = atoi(optarg);
			break;
		case 'v':
			version();
			break;
		case 'w':
			wait_on_exit = 1;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (argc - optind < 1) {
		usage(argv[0]);
	}

	/* If no filename was specified and networking is off, generate a filename */
	if (!out_fname && net_port == -1) {
		fprintf(stderr, "Please specify a file to output to, or -n for networking\n\n");
		usage(argv[0]);
	}
	/*}}}*/

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}

	tui_init(upd_interval);
	tui_print_info("Will read from %s\n", argv[optind]);

	demod = demod_init(raw_samp, interp_factor, costas_bw, symbol_rate);
	demod_start(demod, net_port, out_fname);
	tui_print_info("Demodulator initialized\n");

	while (demod_status(demod)) {
		if (tui_process_input()) {
			break;
		}
		tui_update_file_in(demod_get_perc(demod));
		tui_update_data_out(demod_get_bytes(demod));
		tui_update_pll(demod_get_freq(demod), demod_is_pll_locked(demod));
		tui_draw_constellation(demod_get_buf(demod), 256);
	}

	if (wait_on_exit) {
		tui_print_info("Decoding completed, press any key to exit...\n");
		tui_wait_for_user_input();
	}

	demod_join(demod);
	tui_deinit();

	/* Print some stats about our progress */
	return 0;
}

