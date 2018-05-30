# Meteor-M2 Demodulator

This is a free, open-source LRPT demodulator for the Meteor-M2 Russian weather
satellite. It supports reading from a I/Q recording in .wav format,
and it outputs an 8-bit soft-QPSK file, from which you can generate an image
with the help of LRPTofflineDecoder or
[meteor\_decoder](https://github.com/artlav/meteor_decoder).

Please note that, to get the best decoding performance, you should downsample
the I/Q recording to about 140KHz.

## Compling and installing

As usual, type `make` to compile the project, `make install` to install the
binary to /usr/bin/. A `debug` target is available if you want to keep the debug
symbols in the executable.

## Usage info
```
Usage: meteor_demod [options] file_in
   -o, --output <file>     Output decoded symbols to <file>
   -r, --rate <rate>       Set the symbol rate to <rate> (default: 72000)
   -R, --refresh-rate <ms> Refresh the status screen every <ms> ms (default: 50ms in TUI mode, 5000ms in batch mode)
   -B, --batch             Do not use ncurses, write the message log to stdout instead
   -q, --quiet             Do not print status information

Advanced options:
   -b, --pll-bw <bw>       Set the PLL bandwidth to <bw> (default: 100)
   -f, --fir-order <ord>   Set the RRC filter order to <ord> (default: 64)
   -O, --oversamp <mult>   Set the interpolation factor to <mult> (default: 4)

   -h, --help              Print this help screen
   -v, --version           Print version info
```

Increasing the PLL bandwidth will allow the loop to lock faster to the carrier,
while decreasing it will make the lock more stable.

Increasing the root-raised cosine filter order will slow the decoding down, but
it'll make the filtering more accurate.

Increasing the interpolation factor enables better timing recovery, at the
expense of filtering accuracy. Typically you'll want to increase the RRC order
and the interpolation factor by the same proportion (i.e. multiply them by the
same amount).
