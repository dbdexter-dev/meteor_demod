#include <complex.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "demod.h"
#include "options.h"
#include "tui.h"
#include "utils.h"
#include "wavfile.h"

/* Default values */
#define SYM_RATE 72000
#define INTERP_FACTOR 4
#define SLEEP_INTERVAL 5000

static int stdout_print_info(const char *msg, ...);

int
main(int argc, char *argv[])
{
	int c, free_fname_on_exit;
	struct timespec timespec;
	float in_perc, freq;
	int pll_locked;
	Sample *raw_samp;
	Demod *demod;
	int (*log)(const char *msg, ...);

	/* Command line changeable parameters {{{*/
	int symbol_rate;
	int batch_mode;
	int upd_interval;
	int quiet;
	float costas_bw;
	unsigned interp_factor;
	char *out_fname;
	/*}}}*/
	/* Argument handling {{{ */

	/* Initialize the parameters that can be overridden with command-line args */
	batch_mode  = 0;
	quiet = 0;
	log = tui_print_info;
	upd_interval = UPD_INTERVAL;
	symbol_rate = SYM_RATE;
	costas_bw = COSTAS_BW;
	out_fname = NULL;
	interp_factor = INTERP_FACTOR;
	free_fname_on_exit = 0;

	if (argc < 2) {
		usage(argv[0]);
	}

	/* Parse command line args */
	optind = 0;
	while ((c = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != -1) {
		switch (c) {
		case 'b':
			costas_bw = atoi(optarg);
			break;
		case 'B':
			batch_mode = 1;
			upd_interval = SLEEP_INTERVAL;
			log = stdout_print_info;
			break;
		case 'h':
			usage(argv[0]);
			break;
		case 'o':
			out_fname = optarg;
			break;
		case 'O':
			interp_factor = atoi(optarg);
			break;
		case 'q':
			quiet = 1;
			break;
		case 'r':
			symbol_rate = atoi(optarg);
			break;
		case 'R':
			upd_interval = atoi(optarg);
			break;
		case 'v':
			version();
			break;
		default:
			usage(argv[0]);
		}
	}

	if (argc - optind < 1) {
		usage(argv[0]);
	}
	/*}}}*/
	/* If no filename was specified, generate one */
	if (!out_fname) {
		out_fname = gen_fname();
		free_fname_on_exit = 1;
	}

	/* Open raw samples file */
	raw_samp = open_samples_file(argv[optind]);
	if (!raw_samp) {
		fatal("Couldn't open samples file");
	}

	/* Initialize the UI */
	if (!batch_mode) {
		tui_init(upd_interval);
	} else if (!quiet) {
		splash();
	}

	if (!quiet) {
		log("Will read from %s\n", argv[optind]);
		log("Will output to %s\n", out_fname);
	}

	/* Initialize the demodulator */
	demod = demod_init(raw_samp, interp_factor, costas_bw, symbol_rate);
	demod_start(demod, out_fname);
	if (!quiet) {
		log("Demodulator initialized\n");
	}

	timespec.tv_sec = upd_interval/1000;
	timespec.tv_nsec = ((upd_interval - timespec.tv_sec*1000))*1000L*1000;

	/* Main UI update loop */
	while (demod_status(demod)) {
		in_perc = demod_get_perc(demod);
		freq = demod_get_freq(demod);
		pll_locked = demod_is_pll_locked(demod);

		if (batch_mode) {
			if (!quiet) {
				log("(%5.1f%%) Carrier: %+7.1f Hz, Locked: %s\n",
					in_perc, freq, pll_locked ? "Yes" : "No");
			}
			nanosleep(&timespec, NULL);
		} else {
			if (tui_process_input()) {
				/* Exit on user request */
				break;
			}
			tui_update_file_in(wav_get_size(raw_samp), raw_samp->samplerate, in_perc);
			tui_update_data_out(demod_get_bytes(demod));
			tui_update_pll(freq, pll_locked);
			tui_draw_constellation(demod_get_buf(demod), 256);
		}
	}
	if (!quiet) {
		if (!demod_status(demod)) {
			log("Decoding completed\n");
		} else {
			log("Aborting\n");
		}
	}

	demod_join(demod);
	raw_samp->close(raw_samp);
	if (free_fname_on_exit) {
		free(out_fname);
	}

	if (!batch_mode) {
		log("Press any key to exit...\n");
		tui_wait_for_user_input();
		tui_deinit();
	}

	return 0;
}

/* Static functions {{{*/
int
stdout_print_info(const char *msg, ...)
{
	time_t t;
	va_list ap;
	struct tm* tm;
	char timestr[] = "HH:MM:SS";


	t = time(NULL);
	tm = localtime(&t);
	strftime(timestr, sizeof(timestr), "%T", tm);
	printf("(%s) ", timestr);
	va_start(ap, msg);
	vprintf(msg, ap);
	va_end(ap);

	return 0;
}
/*}}}*/

