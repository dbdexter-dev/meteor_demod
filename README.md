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
   -r, --symrate <rate>    Set the symbol rate to <rate> (default: 72000)
   -s, --samplerate <samp> Force the input samplerate to <samp> (default: auto)
       --bps <bps>         Force the input bits per sample to <bps> (default: 16)
   -R, --refresh-rate <ms> Refresh the status screen every <ms> ms (default: 50ms in TUI mode, 5000ms in batch mode)
   -B, --batch             Do not use ncurses, write the message log to stdout instead
   -q, --quiet             Do not print status information
   -m, --mode <mode>       Specify the signal modulation scheme (default: qpsk)
                           Available modes: qpsk (Meteor-M 2), oqpsk (Meteor-M 2-2)


Advanced options:
   -b, --pll-bw <bw>       Set the PLL bandwidth to <bw> (default: 100)
   -a, --alpha <alpha>     Set the RRC filter alpha to <alpha> (default: 0.6)
   -f, --fir-order <ord>   Set the RRC filter order to <ord> (default: 64)
   -O, --oversamp <mult>   Set the interpolation factor to <mult> (default: 4)

   -h, --help              Print this help screen
   -v, --version           Print version info
```

### Advanced options explanation

Increasing the PLL bandwidth will allow the loop to lock faster to the carrier,
while decreasing it will make the lock more stable.

Increasing the root-raised cosine filter order will slow the decoding down, but
it'll make the filtering more accurate.

Increasing the interpolation factor enables better timing recovery, at the
expense of filtering accuracy. Typically you'll want to increase the RRC order
and the interpolation factor by the same proportion (i.e. multiply them by the
same amount).


## Live decoding

Meteor\_demod now supports live decoding with the help of rtl\_fm! Here's how
to get it working:

First off, you need to create a named pipe. This is simply what the name implies:
a pipe for data. You send data in from one side, and it comes out from the
other. It's going to be used to let rtl\_fm and meteor\_demod exchange samples:
```
mkfifo /tmp/meteor_iq
```
Then, you can start up meteor\_demod, telling it to read the raw samples from
this pipe.  Note the `-s` flag; it *must* be passed because the stream coming from
rtl\_fm doesn't specify a sampling rate, so you need to set it manually:
```
meteor_demod -s 140000 /tmp/meteor_iq
```
This command will seem to hang, but that's expected. The pipe doesn't open
until both the sender and the receiver are connected to it.

So finally, you can start up rtl\_fm to capture the live data and send the raw IQ
stream into the named pipe. Open another terminal and type in:
```
rtl_fm -M raw -s 140000 -f 137.9M -E dc -g <gain> -p <ppm> /tmp/meteor_iq
```
At this point, meteor\_demod's ui should pop up inside the first terminal, and
the constellation diagram should stabilize within 10~15 seconds.

Once the pass is over, stop the decoding by pressing `q`: rtl\_fm will terminate
as well. The named pipe can now be deleted as if it were any other regular file:
```
rm /tmp/meteor_iq
```

You can experiment with the sampling rate, as long as you make sure both rtl\_fm
and meteor\_demod are using the same rate.
