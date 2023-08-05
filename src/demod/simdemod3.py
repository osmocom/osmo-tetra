#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: DQPSK demodulator for Telive, with the AFC packet sending option disabled
# Author: Jacek Lipkowski SQ5BPF
# Description: cqpsk demodulator for tetra. based on  pi4dqpsk_rx.grc (by 'Luca Bortolotti' @optiluca) from https://gitlab.com/larryth/tetra-kit/ . It is meant as a replacement for simdemod2.py, taked the data from stdin, and dumpd demodulated data to STDOUT. Also sends via UDP the information how much the signal is mistuned (this option is disabled for the version committed to osmo-tetra)
# GNU Radio version: 3.10.5.1

from gnuradio import analog
from gnuradio import blocks
from gnuradio import digital
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import cmath
import os




class simdemod3(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "DQPSK demodulator for Telive, with the AFC packet sending option disabled", catch_exceptions=True)

        ##################################################
        # Variables
        ##################################################
        self.sps = sps = 2
        self.nfilts = nfilts = 32
        self.constel = constel = digital.constellation_dqpsk().base()
        self.constel.gen_soft_dec_lut(8)
        self.variable_adaptive_algorithm_0 = variable_adaptive_algorithm_0 = digital.adaptive_algorithm_cma( constel, 10e-3, 1).base()
        self.samp_rate = samp_rate = 2000000
        self.rrc_taps = rrc_taps = firdes.root_raised_cosine(nfilts, nfilts, 1.0/float(sps), 0.35, 11*sps*nfilts)
        self.decim = decim = 32
        self.channel_rate = channel_rate = 36000
        self.arity = arity = 4

        ##################################################
        # Blocks
        ##################################################

        self.stdout_sink = blocks.file_descriptor_sink(gr.sizeof_char*1, 1)
        self.digital_pfb_clock_sync_xxx_0 = digital.pfb_clock_sync_ccf(sps, (2*cmath.pi/100.0), rrc_taps, nfilts, (nfilts/2), 1.5, sps)
        self.digital_map_bb_0 = digital.map_bb(constel.pre_diff_code())
        self.digital_linear_equalizer_0 = digital.linear_equalizer(15, sps, variable_adaptive_algorithm_0, True, [ ], 'corr_est')
        self.digital_fll_band_edge_cc_0 = digital.fll_band_edge_cc(sps, 0.35, 45, (cmath.pi/100.0))
        self.digital_diff_phasor_cc_0 = digital.diff_phasor_cc()
        self.digital_constellation_decoder_cb_0 = digital.constellation_decoder_cb(constel)
        self.blocks_unpack_k_bits_bb_0 = blocks.unpack_k_bits_bb(constel.bits_per_symbol())
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_float*1)
        self.blocks_file_descriptor_source_0 = blocks.file_descriptor_source(gr.sizeof_gr_complex*1, 0, False)
        self.analog_feedforward_agc_cc_0 = analog.feedforward_agc_cc(8, 1)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_feedforward_agc_cc_0, 0), (self.digital_fll_band_edge_cc_0, 0))
        self.connect((self.blocks_file_descriptor_source_0, 0), (self.analog_feedforward_agc_cc_0, 0))
        self.connect((self.blocks_unpack_k_bits_bb_0, 0), (self.stdout_sink, 0))
        self.connect((self.digital_constellation_decoder_cb_0, 0), (self.digital_map_bb_0, 0))
        self.connect((self.digital_diff_phasor_cc_0, 0), (self.digital_constellation_decoder_cb_0, 0))
        self.connect((self.digital_fll_band_edge_cc_0, 2), (self.blocks_null_sink_0, 1))
        self.connect((self.digital_fll_band_edge_cc_0, 3), (self.blocks_null_sink_0, 2))
        self.connect((self.digital_fll_band_edge_cc_0, 1), (self.blocks_null_sink_0, 0))
        self.connect((self.digital_fll_band_edge_cc_0, 0), (self.digital_pfb_clock_sync_xxx_0, 0))
        self.connect((self.digital_linear_equalizer_0, 0), (self.digital_diff_phasor_cc_0, 0))
        self.connect((self.digital_map_bb_0, 0), (self.blocks_unpack_k_bits_bb_0, 0))
        self.connect((self.digital_pfb_clock_sync_xxx_0, 0), (self.digital_linear_equalizer_0, 0))


    def get_sps(self):
        return self.sps

    def set_sps(self, sps):
        self.sps = sps
        self.set_rrc_taps(firdes.root_raised_cosine(self.nfilts, self.nfilts, 1.0/float(self.sps), 0.35, 11*self.sps*self.nfilts))

    def get_nfilts(self):
        return self.nfilts

    def set_nfilts(self, nfilts):
        self.nfilts = nfilts
        self.set_rrc_taps(firdes.root_raised_cosine(self.nfilts, self.nfilts, 1.0/float(self.sps), 0.35, 11*self.sps*self.nfilts))

    def get_constel(self):
        return self.constel

    def set_constel(self, constel):
        self.constel = constel

    def get_variable_adaptive_algorithm_0(self):
        return self.variable_adaptive_algorithm_0

    def set_variable_adaptive_algorithm_0(self, variable_adaptive_algorithm_0):
        self.variable_adaptive_algorithm_0 = variable_adaptive_algorithm_0

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def get_rrc_taps(self):
        return self.rrc_taps

    def set_rrc_taps(self, rrc_taps):
        self.rrc_taps = rrc_taps
        self.digital_pfb_clock_sync_xxx_0.update_taps(self.rrc_taps)

    def get_decim(self):
        return self.decim

    def set_decim(self, decim):
        self.decim = decim

    def get_channel_rate(self):
        return self.channel_rate

    def set_channel_rate(self, channel_rate):
        self.channel_rate = channel_rate

    def get_arity(self):
        return self.arity

    def set_arity(self, arity):
        self.arity = arity




def main(top_block_cls=simdemod3, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    tb.wait()


if __name__ == '__main__':
    main()
