Meteor-M2 series demodulator
============================

This is a free, open-source LRPT demodulator for the Meteor-M2 Russian weather
satellite series. It supports reading from a I/Q recording in .wav format,
and it outputs an 8-bit soft-QPSK file, from which you can generate an image
with the help of LRPTofflineDecoder,
[meteor\_decode](https://github.com/dbdexter-dev/meteor_decode) or
[medet](https://github.com/artlav/meteor_decoder).

Features:
- Support for regular (72k, `-r 72000`) and interleaved (80k, `-r 80000`) modes
- Support for QPSK and OQPSK modulation schemes
- Can read samples from stdin (pass `-` in place of a filename)
- Can output samples to stdout (`--stdout`, disables all status indicators)


Compiling and installing
------------------------

```
mkdir build && cd build
cmake ..
make
sudo make install
```

If you don't need the fancy ncurses interface, you can disable it at compile
time by running `cmake -DENABLE_TUI=OFF ..` when configuring.


Usage info
----------
```
Usage: meteor_demod [options] file_in
           -B, --batch             Disable TUI and all control characters (aka "script-friendly mode")
           -d, --freq-delta <freq> Set the maximum carrier devation to <freq> (default: +-3.5kHz)
           -m, --mode <mode>       Specify the signal modulation scheme (default: qpsk, valid modes: qpsk, oqpsk)
           -o, --output <file>     Output decoded symbols to <file>
           -q, --quiet             Do not print status information
           -r, --symrate <rate>    Set the symbol rate to <rate> (default: 72000)
           -R, --refresh-rate <ms> Refresh the status screen every <ms> ms (default: 50ms in TUI mode, 2000ms in batch mode)
           -s, --samplerate <samp> Force the input samplerate to <samp> (default: auto)
               --bps <bps>         Force the input bits per sample to <bps> (default: 16)
               --stdout            Write output symbols to stdout (implies -B, -q)

           -h, --help              Print this help screen
           -v, --version           Print version info

        Advanced options:
           -b, --pll-bw <bw>       Set the PLL bandwidth to <bw> (default: 1)
           -f, --fir-order <ord>   Set the RRC filter order to <ord> (default: 32)
           -O, --oversamp <mult>   Set the interpolation factor to <mult> (default: 5)
```

Advanced options explanation
----------------------------

- `-b, --pll-bw`: higher = potentially faster carrier acquisition, but worse
  tracking performance if the signal is weak. Does not affect CPU usage.
- `-f, --fir-order`: higher = more accurate signal filtering, but higher CPU usage.
  16-32 is a good range, above 64 is most likely overkill.
- `-O, --oversamp`: higher = more accurate symbol timing recovery, but higher
  CPU usage. Can be reduced if input sampling rate is high, although it's
  more efficient to use a low sampling rate and a high oversampling value than
  vice-versa.


Live demodulation
-----------------
Starting from v1.0, you can live demodulate on a toaster if that's your thing
(~35% peak CPU usage on a Raspberry Pi Zero):

```
rtl_sdr -s 230000 -f 137.1M -g <gain> -p <ppm> - | meteor_demod --bps 8 -s 230000 -B -
```

If you want to see the constellation diagram while demodulating live:

```
mkfifo /tmp/raw_samples
meteor_demod --bps 8 -s 230000 /tmp/raw_samples &
rtl_sdr -s 230000 -f 137.1M -g <gain> -p <ppm> /tmp/raw_samples
rm /tmp/raw_samples
```

With a decoder that supports reading symbols from stdin, you can even decode live
(~75% peak CPU usage on a Raspberry Pi Zero):

```
rtl_sdr -s 230000 -f 137.1M -g <gain> -p <ppm> - | meteor_demod --bps 8 -s 230000 --stdout - | meteor_decode -o live.bmp -
```
