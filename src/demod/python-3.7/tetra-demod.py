#!/usr/bin/env python

import sys
import math
from gnuradio import blocks, filter, gr
from gnuradio.eng_option import eng_option
from optparse import OptionParser

# Load it locally or from the module
try:
    import cqpsk
except:
    from tetra_demod import cqpsk

# accepts an input file in complex format 
# applies frequency translation, resampling (interpolation/decimation)

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)

        parser.add_option("-c", "--calibration", type="eng_float", default=0, help="freq offset")
        parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
        parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
        parser.add_option("-L", "--low-pass", type="eng_float", default=25e3, help="low pass cut-off", metavar="Hz")
        parser.add_option("-o", "--output-file", type="string", default="out.dat", help="specify the output file")
        parser.add_option("-s", "--sample-rate", type="int", default=100000000/512, help="input sample rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        symbol_rate = 18000
        sps = 2
        # output rate will be 36,000
        ntaps = 11 * sps
        new_sample_rate = symbol_rate * sps

        channel_taps = filter.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.1, filter.firdes.WIN_HANN)

        FILTER = filter.freq_xlating_fir_filter_ccf(1, channel_taps, options.calibration, sample_rate)

        sys.stderr.write("sample rate: %d\n" %(sample_rate))

        IN = blocks.file_source(gr.sizeof_gr_complex, options.input_file)

        DEMOD = cqpsk.cqpsk_demod( samples_per_symbol = sps,
                                 excess_bw=0.35,
                                 costas_alpha=0.03,
                                 gain_mu=0.05,
                                 mu=0.05,
                                 omega_relative_limit=0.05,
                                 log=options.log,
                                 verbose=options.verbose)


        OUT = blocks.file_sink(gr.sizeof_float, options.output_file)

        r = float(sample_rate) / float(new_sample_rate)

        INTERPOLATOR = filter.fractional_resampler_cc(0, r)

        self.connect(IN, FILTER, INTERPOLATOR, DEMOD, OUT)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
