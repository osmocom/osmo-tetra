#!/usr/bin/env python

# osmosdr-tetra_demod_fft.py Copyright 2012 Dimitri Stolnikov <horiz0n@gmx.net>

# simdemod.py (c) 2014 Jacek Lipkowski <sq5bpf@lipkowski.org>
# this is a modified osmosdr-tetra_demod_fft.py with all of the gui stuff
# removed. is is intended to be fed from a receiver program via a pipe
# 
# mkfifo /tmp/fifo1
# demod/python/simdemod.py -o /dev/stdout -i /tmp/fifo1 | ./float_to_bits /dev/stdin /dev/stdout | ./tetra-rx /dev/stdin 
#

import sys
import math
from gnuradio import gr, gru, eng_notation, blks2, optfir
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import osmosdr

try:
    import cqpsk
except:
    from tetra_demod import cqpsk

# applies frequency translation, resampling and demodulation

class top_block(gr.top_block):
  def __init__(self):
    gr.top_block.__init__(self, "Top Block")

    options = get_options()
    self.input_file=options.input_file
    self.gr_file_source_0 = gr.file_source(gr.sizeof_gr_complex*1, self.input_file, True)

    symbol_rate = 18000
    sps = 2 # output rate will be 36,000
    out_sample_rate = symbol_rate * sps

    options.low_pass = options.low_pass / 2.0

    self.demod = cqpsk.cqpsk_demod(
        samples_per_symbol = sps,
        excess_bw=0.35,
        costas_alpha=0.03,
        gain_mu=0.05,
        mu=0.05,
        omega_relative_limit=0.05,
        log=options.log,
        verbose=options.verbose)

    self.output = gr.file_sink(gr.sizeof_float, options.output_file)

    self.connect(self.gr_file_source_0, self.demod, self.output)






def get_options():
    parser = OptionParser(option_class=eng_option)

    # demodulator related settings
    parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
    parser.add_option("-i", "--input-file", type="string", default="in.float", help="specify the bit input file")
    parser.add_option("-o", "--output-file", type="string", default="out.float", help="specify the bit output file")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
    parser.add_option("-L", "--low-pass", type="eng_float", default=25e3, help="low pass cut-off", metavar="Hz")

    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        raise SystemExit, 1

    return (options)

if __name__ == '__main__':
        tb = top_block()
        tb.run()
        #tb.run(True)
