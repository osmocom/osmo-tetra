#!/usr/bin/env python

import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir
from gnuradio import usrp
from gnuradio.eng_option import eng_option
from optparse import OptionParser

try:
    import cqpsk
except:
    from tetra_demod import cqpsk

# applies frequency translation, resampling (interpolation/decimation) and cqpsk demodulation

class my_top_block(gr.top_block):
    def __init__(self, options):
        gr.top_block.__init__(self)

        fusb_block_size = gr.prefs().get_long('fusb', 'block_size', 4096)
        fusb_nblocks    = gr.prefs().get_long('fusb', 'nblocks', 16)
        self._u = usrp.source_c(decim_rate=options.decim, fusb_block_size=fusb_block_size, fusb_nblocks=fusb_nblocks)

        if options.rx_subdev_spec is None:
            #options.rx_subdev_spec = pick_subdevice(self._u)
            options.rx_subdev_spec = (0, 0)

        self._u.set_mux(usrp.determine_rx_mux_value(self._u, options.rx_subdev_spec))
        # determine the daughterboard subdevice
        self.subdev = usrp.selected_subdev(self._u, options.rx_subdev_spec)

        # set initial values
        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            g = self.subdev.gain_range()
            options.gain = float(g[0]+g[1])/2

        r = self._u.tune(0, self.subdev, options.freq)
        self.subdev.set_gain(options.gain)

        #sample_rate = options.fpga_clock/options.decim
        sample_rate = self._u.adc_freq() / self._u.decim_rate()
        symbol_rate = 18000
        sps = 2
        # output rate will be 36,000
        ntaps = 11 * sps
        new_sample_rate = symbol_rate * sps

        channel_taps = gr.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.1, gr.firdes.WIN_HANN)

        FILTER = gr.freq_xlating_fir_filter_ccf(1, channel_taps, options.calibration, sample_rate)

        sys.stderr.write("sample rate: %d\n" %(sample_rate))

        DEMOD = cqpsk.cqpsk_demod( samples_per_symbol = sps,
                                 excess_bw=0.35,
                                 costas_alpha=0.03,
                                 gain_mu=0.05,
                                 mu=0.05,
                                 omega_relative_limit=0.05,
                                 log=options.log,
                                 verbose=options.verbose)

        OUT = gr.file_sink(gr.sizeof_float, options.output_file)

        r = float(sample_rate) / float(new_sample_rate)

        INTERPOLATOR = gr.fractional_interpolator_cc(0, r)

        self.connect(self._u, FILTER, INTERPOLATOR, DEMOD, OUT)

def get_options():
    parser = OptionParser(option_class=eng_option)

    # usrp related settings
    parser.add_option("-d", "--decim", type="int", default=250,
                      help="Set USRP decimation rate to DECIM [default=%default]")
    parser.add_option("-f", "--freq", type="eng_float", default=None,
                      help="set frequency to FREQ", metavar="FREQ")
    parser.add_option("-g", "--gain", type="eng_float", default=None,
                      help="set gain in dB (default is midpoint)")
    parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                      help="Select USRP Rx side A or B (default=first one with a daughterboard)")

    # demodulator related settings
    parser.add_option("-c", "--calibration", type="int", default=0, help="freq offset")
    parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
    parser.add_option("-L", "--low-pass", type="eng_float", default=25e3, help="low pass cut-off", metavar="Hz")
    parser.add_option("-o", "--output-file", type="string", default="out.float", help="specify the bit output file")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")

    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        raise SystemExit, 1
    
    if options.freq is None:
        parser.print_help()
        sys.stderr.write('You must specify the frequency with -f FREQ\n');
        raise SystemExit, 1
    
    return (options)

if __name__ == "__main__":
    (options) = get_options()
    tb = my_top_block(options)
    try:
        tb.run()
    except KeyboardInterrupt:
        tb.stop()
