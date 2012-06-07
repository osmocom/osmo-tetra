#!/usr/bin/env python

# Copyright 2012 Dimitri Stolnikov <horiz0n@gmx.net>

# Usage:
# src$ ./demod/python/rtlsdr-tetra_demod_fft.py -o /dev/stdout | ./float_to_bits /dev/stdin /dev/stdout | ./tetra-rx /dev/stdin
#
# Adjust the center frequency (-f) and gain (-g) according to your needs.
# Use left click in Wideband Spectrum window to roughly select a TETRA carrier.
# In Wideband Spectrum you can also tune by 1/4 of the bandwidth by clicking on the rightmost/leftmost spectrum side.
# Use left click in Channel Spectrum windows to fine tune the carrier by clicking on the left or right side of the spectrum.


import sys
import math
from gnuradio import gr, gru, eng_notation, blks2, optfir
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import osmosdr
import wx

try:
    import cqpsk
except:
    from tetra_demod import cqpsk

# applies frequency translation, resampling and demodulation

class top_block(grc_wxgui.top_block_gui):
  def __init__(self):
    grc_wxgui.top_block_gui.__init__(self, title="Top Block")

    options = get_options()

    self.ifreq = options.frequency
    self.rfgain = options.gain

    self.src = osmosdr.source_c()
    self.src.set_center_freq(self.ifreq)
    self.src.set_sample_rate(int(options.sample_rate))

    if self.rfgain is None:
        self.src.set_gain_mode(1)
        self.iagc = 1
        self.rfgain = 0
    else:
        self.iagc = 0
        self.src.set_gain_mode(0)
        self.src.set_gain(self.rfgain)

    # may differ from the requested rate
    sample_rate = self.src.get_sample_rate()

    symbol_rate = 18000
    sps = 2 # output rate will be 36,000
    out_sample_rate = symbol_rate * sps

    options.low_pass = options.low_pass / 2.0

    first_decim = 10

    self.offset = 0

    taps = gr.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.2, gr.firdes.WIN_HANN)
    self.tuner = gr.freq_xlating_fir_filter_ccf(first_decim, taps, self.offset, sample_rate)

    sys.stderr.write("sample rate: %d\n" % (sample_rate))

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

    rerate = float(sample_rate / float(first_decim)) / float(out_sample_rate)
    sys.stderr.write("resampling factor: %f\n" % rerate)

    if rerate.is_integer():
        sys.stderr.write("using pfb decimator\n")
        self.resamp = blks2.pfb_decimator_ccf(int(rerate))
    else:
        sys.stderr.write("using pfb resampler\n")
        self.resamp = blks2.pfb_arb_resampler_ccf(1 / rerate)

    self.connect(self.src, self.tuner, self.resamp, self.demod, self.output)

    def set_ifreq(ifreq):
        self.ifreq = ifreq
        self._ifreq_text_box.set_value(self.ifreq)
        self.src.set_center_freq(self.ifreq)

    self._ifreq_text_box = forms.text_box(
        parent=self.GetWin(),
        value=self.ifreq,
        callback=set_ifreq,
        label="Center Frequency",
        converter=forms.float_converter(),
    )
    self.Add(self._ifreq_text_box)

    def set_iagc(iagc):
        self.iagc = iagc
        self._agc_check_box.set_value(self.iagc)
        self.src.set_gain_mode(self.iagc, 0)
        self.src.set_gain(0 if self.iagc == 1 else self.rfgain, 0)

    self._agc_check_box = forms.check_box(
        parent=self.GetWin(),
        value=self.iagc,
        callback=set_iagc,
        label="Automatic Gain",
        true=1,
        false=0,
    )

    self.Add(self._agc_check_box)

    def set_rfgain(rfgain):
        self.rfgain = rfgain
        self._rfgain_slider.set_value(self.rfgain)
        self._rfgain_text_box.set_value(self.rfgain)
        self.src.set_gain(0 if self.iagc == 1 else self.rfgain, 0)

    _rfgain_sizer = wx.BoxSizer(wx.VERTICAL)
    self._rfgain_text_box = forms.text_box(
        parent=self.GetWin(),
        sizer=_rfgain_sizer,
        value=self.rfgain,
        callback=set_rfgain,
        label="RF Gain",
        converter=forms.float_converter(),
        proportion=0,
    )
    self._rfgain_slider = forms.slider(
        parent=self.GetWin(),
        sizer=_rfgain_sizer,
        value=self.rfgain,
        callback=set_rfgain,
        minimum=0,
        maximum=50,
        num_steps=200,
        style=wx.SL_HORIZONTAL,
        cast=float,
        proportion=1,
    )

    self.Add(_rfgain_sizer)

    def fftsink2_callback(x, y):
        if abs(x / (sample_rate / 2)) > 0.9:
            set_ifreq(self.ifreq + x / 2)
        else:
            sys.stderr.write("coarse tuned to: %d Hz\n" % x)
            self.offset = -x
            self.tuner.set_center_freq(self.offset)

    self.scope = fftsink2.fft_sink_c(self.GetWin(),
        title="Wideband Spectrum (click to coarse tune)",
        fft_size=1024,
        sample_rate=sample_rate,
        ref_scale=2.0,
        ref_level=0,
        y_divs=10,
        fft_rate=10,
        average=False,
        avg_alpha=0.6)

    self.Add(self.scope.win)
    self.scope.set_callback(fftsink2_callback)

    self.connect(self.src, self.scope)

    def fftsink2_callback2(x, y):
        self.offset = self.offset - (x / 10)
        sys.stderr.write("fine tuned to: %d Hz\n" % self.offset)
        self.tuner.set_center_freq(self.offset)

    self.scope2 = fftsink2.fft_sink_c(self.GetWin(),
        title="Channel Spectrum (click to fine tune)",
        fft_size=1024,
        sample_rate=out_sample_rate,
        ref_scale=2.0,
        ref_level=-20,
        y_divs=10,
        fft_rate=10,
        average=False,
        avg_alpha=0.6)

    self.Add(self.scope2.win)
    self.scope2.set_callback(fftsink2_callback2)

    self.connect(self.resamp, self.scope2)

def get_options():
    parser = OptionParser(option_class=eng_option)

    parser.add_option("-s", "--sample-rate", type="eng_float", default=1800000,
        help="set receiver sample rate (default 1800000)")
    parser.add_option("-f", "--frequency", type="eng_float", default=394.6e6,
        help="set receiver center frequency")
    parser.add_option("-g", "--gain", type="eng_float", default=None,
        help="set receiver gain")

    # demodulator related settings
    parser.add_option("-l", "--log", action="store_true", default=False, help="dump debug .dat files")
    parser.add_option("-L", "--low-pass", type="eng_float", default=25e3, help="low pass cut-off", metavar="Hz")
    parser.add_option("-o", "--output-file", type="string", default="out.float", help="specify the bit output file")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        raise SystemExit, 1

    return (options)

if __name__ == '__main__':
        tb = top_block()
        tb.Run(True)
