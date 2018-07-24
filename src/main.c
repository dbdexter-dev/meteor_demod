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

/* Default values #defines {{{ */
/* Default symbol rate */
#define SYM_RATE 72000

/* Default update intervals */
#define SLEEP_INTERVAL 5000
#define UPD_INTERVAL 50

/* Costas loop default parameters */
#define COSTAS_BW 100

/* RRC default parameters, alpha taken from the .grc meteor decode script */
#define RRC_ALPHA 0.6
#define RRC_FIR_ORDER 64

/* Interpolator default options */
#define INTERP_FACTOR 4
/*}}}*/

static int stdout_print_info(const char *msg, ...);

int
main(int argc, char *argv[])
{
	int c, free_fname_on_exit;
	struct timespec timespec;
	float freq, gain;
	uint64_t in_done, in_total;
	int pll_locked;
	Source *raw_samp;
	Demod *demod;

	/* Command line changeable parameters {{{*/
	int symbol_rate;
	unsigned samplerate;
	int batch_mode;
	int upd_interval;
	int quiet;
	float costas_bw;
	float rrc_alpha;
	unsigned interp_factor;
	unsigned rrc_order;
	char *out_fname;
	int (*log)(const char *msg, ...);
	/*}}}*/
	/* Initialize the parameters that can be overridden with command-line args {{{*/
	batch_mode  = 0;
	rrc_alpha = RRC_ALPHA;
	samplerate = 0;
	quiet = 0;
	log = tui_print_info;
	upd_interval = UPD_INTERVAL;
	symbol_rate = SYM_RATE;
	costas_bw = COSTAS_BW;
	out_fname = NULL;
	interp_factor = INTERP_FACTOR;
	rrc_order = RRC_FIR_ORDER;
	free_fname_on_exit = 0;
	/* }}} */
	/* Parse command line args {{{*/
	if (argc < 2) {
		usage(argv[0]);
	}

	optind = 0;
	while ((c = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != -1) {
		switch (c) {
		case 'a':
			rrc_alpha = atof(optarg);
			break;
		case 'b':
			costas_bw = atoi(optarg);
			break;
		case 'B':
			batch_mode = 1;
			upd_interval = SLEEP_INTERVAL;
			log = stdout_print_info;
			break;
		case 'f':
			rrc_order = atoi(optarg);
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
		case 's':
			samplerate = atoi(optarg);
			break;
		case 'v':
			version();
			break;
		default:
			usage(argv[0]);
		}
	}

	/* Check if input filename was provided */
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
	raw_samp = open_samples_file(argv[optind], samplerate);
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
		log("Input: %s, output: %s\n", argv[optind], out_fname);
		log("Input samplerate: %d\n", raw_samp->samplerate);
	}

	/* Initialize the demodulator */
	demod = demod_init(raw_samp, interp_factor, rrc_order, rrc_alpha, costas_bw, symbol_rate);
	demod_start(demod, out_fname);
	if (!quiet) {
		log("Demodulator initialized\n");
	}

	/* Initialize the struct that will be the argument to nanosleep() */
	timespec.tv_sec = upd_interval/1000;
	timespec.tv_nsec = ((upd_interval - timespec.tv_sec*1000))*1000L*1000;

	/* Main UI update loop */
	in_total = demod_get_size(demod);
	while (demod_status(demod)) {
		in_done = demod_get_done(demod);
		freq = demod_get_freq(demod);
		gain = demod_get_gain(demod);
		pll_locked = demod_is_pll_locked(demod);

		if (batch_mode) {
			if (!quiet) {
				log("(%5.1f%%) Carrier: %+7.1f Hz, Locked: %s\n",
					(float)in_done/in_total*100, freq, pll_locked ? "Yes" : "No");
			}
			nanosleep(&timespec, NULL);
		} else {
			if (tui_process_input()) {
				/* Exit on user request */
				break;
			}
			tui_update_file_in(raw_samp->samplerate, in_done, in_total);
			tui_update_data_out(demod_get_bytes_out(demod));
			tui_update_pll(freq, pll_locked, gain);
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

