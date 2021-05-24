#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "demod.h"
#include "dsp/timing.h"
#include "utils.h"
#include "wavfile.h"
#ifdef ENABLE_TUI
#include <ncurses.h>
#include "tui.h"
#endif

#define SHORTOPTS "a:Bb:f:hm:o:O:qR:r:s:S:v"
#define RINGSIZE 512

struct thropts {
	FILE *samples_file, *soft_file;
	int bps;
	int (*demod)(float complex *sample);
	int done;
	unsigned long bytes_out;
	pthread_t main_tid;
};

static void* thread_process(void *parms);
static void noop(int x) { return; }

static int8_t _symbols_ring[2*RINGSIZE];
static struct option longopts[] = {
	{ "batch",        0, NULL, 'B' },
	{ "pll-bw",       1, NULL, 'b' },
	{ "diff",         0, NULL, 'd' },
	{ "fir-order",    1, NULL, 'f' },
	{ "help",         0, NULL, 'h' },
	{ "mode",         1, NULL, 'm' },
	{ "output",       1, NULL, 'o' },
	{ "oversamp",     1, NULL, 'O' },
	{ "quiet",        0, NULL, 'q' },
	{ "refresh-rate", 1, NULL, 'R' },
	{ "symrate",      1, NULL, 'r' },
	{ "stdout",       0, NULL, 0x00},
	{ "samplerate",   1, NULL, 's' },
	{ "bps",          1, NULL, 'S' },
	{ "version",      0, NULL, 'v' },
};


int
main(int argc, char *argv[])
{
	unsigned long file_len, tmp;
	FILE *samples_file, *soft_file;
	int c, i, j=0;
	volatile struct thropts thread_args;
	pthread_t tid;
	struct timespec sleep_timespec;
	float freq_hz, rate_hz;

	/* Command-line changeable parameters {{{ */
	float pll_bw = PLL_BW;
	int rrc_order = RRC_ORDER;
	int interp_factor = INTERP_FACTOR;
	int quiet = 0;
	float symrate = SYM_RATE;
	int (*demod)(float complex *sample) = demod_qpsk;
	int (*message)(const char *fmt, ...) = printf;
	int batch = 0;
	int update_interval = -1;
	int bps = 0;
	int samplerate = -1;
	int stdout_mode = 0;
	char *output_fname = NULL;
	/* }}} */
	/* Parse command-line options {{{ */
	while ((c = getopt_long(argc, argv, SHORTOPTS, longopts, NULL)) != -1) {
		switch (c) {
			case 0x00:
				/* Stdout mode */
				stdout_mode = 1;
				break;
			case 'b':
				pll_bw = atof(optarg);
				break;
			case 'B':
				batch = 1;
				break;
			case 'f':
				rrc_order = atoi(optarg);
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			case 'm':
				if (!strcmp(optarg, "oqpsk")) demod = demod_oqpsk;
				break;
			case 'o':
				output_fname = optarg;
				break;
			case 'O':
				interp_factor = atoi(optarg);
				break;
			case 'q':
				quiet = 1;
				break;
			case 'R':
				update_interval = atoi(optarg);
				break;
			case 'r':
				symrate = atof(optarg);
				break;
			case 's':
				samplerate = atoi(optarg);
				break;
			case 'S':
				bps = atoi(optarg);
				break;
			case 'v':
				version();
				return 0;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (argc - optind < 1) {
		usage(argv[0]);
		return 1;
	}

	if (!output_fname) output_fname = gen_fname();
	if (update_interval < 0) update_interval = batch ? 2000 : 50;
	if (stdout_mode) {
		batch = 1;
		quiet = 1;
	}
#ifdef ENABLE_TUI
	message = batch ? printf : tui_print_info;
#endif
	/* }}} */

	/* Open input file */
	if (!strcmp(argv[optind], "-")) {
		samples_file = stdin;
		batch = 1;          /* Ncurses doesn't play nice with stdin samples */
	} else if (!(samples_file = fopen(argv[optind], "rb"))) {
		fprintf(stderr, "Could not open input file\n");
		return 1;
	}

	/* Parse wav header. If it fails, assume raw data */
	if (wav_parse(samples_file, &samplerate, &bps)) {
		fseek(samples_file, 0, SEEK_SET);
	}

	if (samplerate < 0) {
		fprintf(stderr, "Could not auto-detect sample rate. Please specify it with -s <samplerate>\n");
		usage(argv[0]);
		return 1;
	}
	if (!bps) {
		fprintf(stderr, "Could not auto-detect bits per sample, assuming 16\n");
		bps = 16;
	}

	/* Open output file */
	if (stdout_mode) {
		soft_file = stdout;
	} else if (!(soft_file = fopen(output_fname, "wb"))) {
		fprintf(stderr, "Could not open output file\n");
		return 1;
	}

	/* Initialize subsystems */
	demod_init(pll_bw, SYM_BW, samplerate, symrate, interp_factor, rrc_order, demod == demod_oqpsk);

	/* Get file length */
	tmp = ftell(samples_file);
	fseek(samples_file, 0, SEEK_END);
	file_len = ftell(samples_file);
	fseek(samples_file, tmp, SEEK_SET);

#ifdef ENABLE_TUI
	if (!batch) tui_init(update_interval);
#endif

	if (!quiet) message("Input: %s, output: %s\n", argv[optind], output_fname);

	/* Prepare thread arguments */
	thread_args.samples_file = samples_file;
	thread_args.soft_file = soft_file;
	thread_args.bps = bps;
	thread_args.demod = demod;
	thread_args.done = 0;
	thread_args.main_tid = pthread_self();

	sleep_timespec.tv_sec = update_interval / 1000;
	sleep_timespec.tv_nsec = (update_interval % 1000) * 1000L * 1000;

	/* SIGUSR1 is used just to wake up the main thread when the demod thread
	 * exits, so connect it to a no-op handler */
	signal(SIGUSR1, &noop);

	/* Launch demod thread */
	pthread_create(&tid, NULL, thread_process, (void*)&thread_args);
	if (!quiet) message("Demodulator initialized\n");

#ifdef ENABLE_TUI
	if (!batch) {
		/* TUI mode: update ncurses screen until done */
		while (!thread_args.done) {
			/* Exit on user request. Also throttles refresh rate */
			if (tui_process_input()) {
				thread_args.done = 1;
				break;
			}

			freq_hz = pll_get_freq()*symrate/(2*M_PI)*(demod == demod_oqpsk ? 2 : 1);
			rate_hz = mm_omega()*(samplerate*interp_factor)/(2*M_PI);

			/* Update TUI */
			tui_update_file_in(2*samplerate*bps/8, ftell(samples_file), file_len);
			tui_update_data_out(thread_args.bytes_out);
			tui_update_pll(freq_hz, rate_hz, pll_get_locked(), agc_get_gain());
			tui_draw_constellation(_symbols_ring, LEN(_symbols_ring));
		}

		message("Demodulation complete\n");
		message("Press any key to exit...\n");
		tui_wait_for_user_input();
		tui_deinit();
	} else {
#endif
	if (!quiet) {
		/* Batch mode: periodically write status line */
		while (!thread_args.done) {
			freq_hz = pll_get_freq()*symrate/(2*M_PI)*(demod == demod_oqpsk ? 2 : 1);
			rate_hz = mm_omega()*(samplerate*interp_factor)/(2*M_PI);

			message(batch ? "\n" : "\033[1K\r");
			message("(%5.1f%%) Carrier: %+7.1f Hz, Symbol rate: %.1f Hz, Locked: %s",
				   file_len ? 100.0 * ftell(samples_file)/file_len : 0,
				   freq_hz,
				   rate_hz,
				   pll_get_locked() ? "Yes" : "No");
			fflush(stdout);
			nanosleep(&sleep_timespec, NULL);
		}
		printf("\n");
	}

#ifdef ENABLE_TUI
	}       /* if (!batch) else */
#endif

	/* Join demod thread */
	pthread_join(tid, NULL);

	/* Cleanup */
	demod_deinit();
	if (soft_file != stdout) fclose(soft_file);
	if (samples_file != stdin) fclose(samples_file);

	return 0;
}

/* Static functions {{{ */
/**
 * Core processing functionality, split into a function for easy threading
 */
static void*
thread_process(void *x)
{
	float complex sample;
	FILE *samples_file, *soft_file;
	int bps;
	int (*demod)(float complex *sample);
	unsigned ring_idx;
	volatile struct thropts *parms = (struct thropts *)x;

	samples_file = parms->samples_file;
	soft_file = parms->soft_file;
	bps = parms->bps;
	demod = parms->demod;
	ring_idx = 0;


	/* Main processing loop */
	parms->bytes_out = 0;
	while (!parms->done && wav_read(&sample, bps, samples_file)) {
		if (demod(&sample)) {
			_symbols_ring[ring_idx++] = MAX(-127, MIN(127, crealf(sample)/2));
			_symbols_ring[ring_idx++] = MAX(-127, MIN(127, cimagf(sample)/2));

			if (ring_idx >= LEN(_symbols_ring)) {
				/* Only write symbols after the PLL locked once */
				if (pll_did_lock_once()) fwrite(_symbols_ring, RINGSIZE, 2, soft_file);
				ring_idx = 0;
				parms->bytes_out += LEN(_symbols_ring);
			}
		}
	}

	/* Flush output buffer */
	fwrite(_symbols_ring, ring_idx, 2, soft_file);
	parms->bytes_out += ring_idx;
	parms->done = 1;

	/* Wake up main thread */
	pthread_kill(parms->main_tid, SIGUSR1);

	return NULL;
}
/* }}} */
