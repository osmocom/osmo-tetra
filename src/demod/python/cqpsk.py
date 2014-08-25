#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
#
# cqpsk.py (C) Copyright 2009, KA1RBI
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

# See gnuradio-examples/python/digital for examples

"""
differential PI/4 CQPSK modulation and demodulation.
"""

from gnuradio import gr, gru, blocks, analog, filter, digital
from gnuradio.filter import firdes
from math import pi, sqrt
#import psk
import cmath
from pprint import pprint

_def_has_gr_digital = False

# address gnuradio 3.5.x changes
try:
    from gnuradio import modulation_utils
except ImportError:
    from gnuradio import digital
    _def_has_gr_digital = True

# default values (used in __init__ and add_options)
_def_samples_per_symbol = 10
_def_excess_bw = 0.35
_def_gray_code = True
_def_verbose = False
_def_log = False

_def_costas_alpha = 0.15
_def_gain_mu = None
_def_mu = 0.5
_def_omega_relative_limit = 0.005


# /////////////////////////////////////////////////////////////////////////////
#                           CQPSK modulator
# /////////////////////////////////////////////////////////////////////////////

class cqpsk_mod(gr.hier_block2):

    def __init__(self,
                 samples_per_symbol=_def_samples_per_symbol,
                 excess_bw=_def_excess_bw,
                 verbose=_def_verbose,
                 log=_def_log):
        """
	Hierarchical block for RRC-filtered QPSK modulation.

	The input is a byte stream (unsigned char) and the
	output is the complex modulated signal at baseband.

	@param samples_per_symbol: samples per symbol >= 2
	@type samples_per_symbol: integer
	@param excess_bw: Root-raised cosine filter excess bandwidth
	@type excess_bw: float
        @param verbose: Print information about modulator?
        @type verbose: bool
        @param debug: Print modualtion data to files?
        @type debug: bool
	"""

	gr.hier_block2.__init__(self, "cqpsk_mod",
				gr.io_signature(1, 1, gr.sizeof_char),       # Input signature
				gr.io_signature(1, 1, gr.sizeof_gr_complex)) # Output signature

        self._samples_per_symbol = samples_per_symbol
        self._excess_bw = excess_bw

        if not isinstance(samples_per_symbol, int) or samples_per_symbol < 2:
            raise TypeError, ("sbp must be an integer >= 2, is %d" % samples_per_symbol)

	ntaps = 11 * samples_per_symbol
 
        arity = 8

        # turn bytes into k-bit vectors
        self.bytes2chunks = \
          gr.packed_to_unpacked_bb(self.bits_per_symbol(), gr.GR_MSB_FIRST)

        #	0	+45	1	[+1]
        #	1	+135	3	[+3]
        #	2	-45	7	[-1]
        #	3	-135	5	[-3]
        self.pi4map = [1, 3, 7, 5]
        self.symbol_mapper = gr.map_bb(self.pi4map)
        self.diffenc = gr.diff_encoder_bb(arity)
        self.chunks2symbols = gr.chunks_to_symbols_bc(psk.constellation[arity])

        # pulse shaping filter
	self.rrc_taps = firdes.root_raised_cosine(
	    self._samples_per_symbol, # gain  (sps since we're interpolating by sps)
            self._samples_per_symbol, # sampling rate
            1.0,		      # symbol rate
            self._excess_bw,          # excess bandwidth (roll-off factor)
            ntaps)

	self.rrc_filter = filter.interp_fir_filter_ccf(self._samples_per_symbol, self.rrc_taps)

        if verbose:
            self._print_verbage()
        
        if log:
            self._setup_logging()
            
	# Connect & Initialize base class
        self.connect(self, self.bytes2chunks, self.symbol_mapper, self.diffenc,
                     self.chunks2symbols, self.rrc_filter, self)

    def samples_per_symbol(self):
        return self._samples_per_symbol

    def bits_per_symbol(self=None):   # staticmethod that's also callable on an instance
        return 2
    bits_per_symbol = staticmethod(bits_per_symbol)      # make it a static method.  RTFM

    def _print_verbage(self):
        print "\nModulator:"
        print "bits per symbol:     %d" % self.bits_per_symbol()
        print "Gray code:           %s" % self._gray_code
        print "RRS roll-off factor: %f" % self._excess_bw

    def _setup_logging(self):
        print "Modulation logging turned on."
        self.connect(self.bytes2chunks,
                     blocks.file_sink(gr.sizeof_char, "tx_bytes2chunks.dat"))
        self.connect(self.symbol_mapper,
                     blocks.file_sink(gr.sizeof_char, "tx_graycoder.dat"))
        self.connect(self.diffenc,
                     blocks.file_sink(gr.sizeof_char, "tx_diffenc.dat"))        
        self.connect(self.chunks2symbols,
                     blocks.file_sink(gr.sizeof_gr_complex, "tx_chunks2symbols.dat"))
        self.connect(self.rrc_filter,
                     blocks.file_sink(gr.sizeof_gr_complex, "tx_rrc_filter.dat"))

    def add_options(parser):
        """
        Adds QPSK modulation-specific options to the standard parser
        """
        parser.add_option("", "--excess-bw", type="float", default=_def_excess_bw,
                          help="set RRC excess bandwith factor [default=%default] (PSK)")
        parser.add_option("", "--no-gray-code", dest="gray_code",
                          action="store_false", default=_def_gray_code,
                          help="disable gray coding on modulated bits (PSK)")
    add_options=staticmethod(add_options)


    def extract_kwargs_from_options(options):
        """
        Given command line options, create dictionary suitable for passing to __init__
        """
        return modulation_utils.extract_kwargs_from_options(dqpsk_mod.__init__,
                                                            ('self',), options)
    extract_kwargs_from_options=staticmethod(extract_kwargs_from_options)


# /////////////////////////////////////////////////////////////////////////////
#                           CQPSK demodulator
#
# /////////////////////////////////////////////////////////////////////////////

class cqpsk_demod(gr.hier_block2):

    def __init__(self, 
                 samples_per_symbol=_def_samples_per_symbol,
                 excess_bw=_def_excess_bw,
                 costas_alpha=_def_costas_alpha,
                 gain_mu=_def_gain_mu,
                 mu=_def_mu,
                 omega_relative_limit=_def_omega_relative_limit,
                 gray_code=_def_gray_code,
                 verbose=_def_verbose,
                 log=_def_log):
        """
	Hierarchical block for RRC-filtered CQPSK demodulation

	The input is the complex modulated signal at baseband.
	The output is a stream of floats in [ -3 / -1 / +1 / +3 ]

	@param samples_per_symbol: samples per symbol >= 2
	@type samples_per_symbol: float
	@param excess_bw: Root-raised cosine filter excess bandwidth
	@type excess_bw: float
        @param costas_alpha: loop filter gain
        @type costas_alphas: float
        @param gain_mu: for M&M block
        @type gain_mu: float
        @param mu: for M&M block
        @type mu: float
        @param omega_relative_limit: for M&M block
        @type omega_relative_limit: float
        @param gray_code: Tell modulator to Gray code the bits
        @type gray_code: bool
        @param verbose: Print information about modulator?
        @type verbose: bool
        @param debug: Print modualtion data to files?
        @type debug: bool
	"""

	gr.hier_block2.__init__(self, "cqpsk_demod",
			        gr.io_signature(1, 1, gr.sizeof_gr_complex), # Input signature
			        gr.io_signature(1, 1, gr.sizeof_float))       # Output signature

        self._samples_per_symbol = samples_per_symbol
        self._excess_bw = excess_bw
        self._costas_alpha = costas_alpha
        self._mm_gain_mu = gain_mu
        self._mm_mu = mu
        self._mm_omega_relative_limit = omega_relative_limit
        self._gray_code = gray_code

        if samples_per_symbol < 2:
            raise TypeError, "sbp must be >= 2, is %d" % samples_per_symbol

        arity = pow(2,self.bits_per_symbol())
 
        # Automatic gain control
        scale = (1.0/16384.0)
        self.pre_scaler = blocks.multiply_const_cc(scale)   # scale the signal from full-range to +-1
        #self.agc = gr.agc2_cc(0.6e-1, 1e-3, 1, 1, 100)
        self.agc = analog.feedforward_agc_cc(16, 2.0)
       
        # RRC data filter
        ntaps = 11 * samples_per_symbol
        self.rrc_taps = firdes.root_raised_cosine(
            1.0,                      # gain
            self._samples_per_symbol, # sampling rate
            1.0,                      # symbol rate
            self._excess_bw,          # excess bandwidth (roll-off factor)
            ntaps)
        self.rrc_filter=filter.interp_fir_filter_ccf(1, self.rrc_taps)        

        if not self._mm_gain_mu:
            sbs_to_mm = {2: 0.050, 3: 0.075, 4: 0.11, 5: 0.125, 6: 0.15, 7: 0.15}
            self._mm_gain_mu = sbs_to_mm[samples_per_symbol]

        self._mm_omega = self._samples_per_symbol
        self._mm_gain_omega = .25 * self._mm_gain_mu * self._mm_gain_mu
        self._costas_beta  = 0.25 * self._costas_alpha * self._costas_alpha
        fmin = -0.025
        fmax = 0.025
        
	if not _def_has_gr_digital:
            self.receiver=gr.mpsk_receiver_cc(arity, pi/4.0,
                                          self._costas_alpha, self._costas_beta,
                                          fmin, fmax,
                                          self._mm_mu, self._mm_gain_mu,
                                          self._mm_omega, self._mm_gain_omega,
                                          self._mm_omega_relative_limit)
	else:
            self.receiver=digital.mpsk_receiver_cc(arity, pi/4.0,
                                          2*pi/150,
                                          fmin, fmax,
                                          self._mm_mu, self._mm_gain_mu,
                                          self._mm_omega, self._mm_gain_omega,
                                          self._mm_omega_relative_limit)

	    self.receiver.set_alpha(self._costas_alpha)
	    self.receiver.set_beta(self._costas_beta)

        # Perform Differential decoding on the constellation
        self.diffdec = digital.diff_phasor_cc()

        # take angle of the difference (in radians)
        self.to_float = blocks.complex_to_arg()

        # convert from radians such that signal is in -3/-1/+1/+3
        self.rescale = blocks.multiply_const_ff( 1 / (pi / 4) )

        if verbose:
            self._print_verbage()
        
        if log:
            self._setup_logging()
 
        # Connect & Initialize base class
        self.connect(self, self.pre_scaler, self.agc, self.rrc_filter, self.receiver,
                     self.diffdec, self.to_float, self.rescale, self)

    def samples_per_symbol(self):
        return self._samples_per_symbol

    def bits_per_symbol(self=None):   # staticmethod that's also callable on an instance
        return 2
    bits_per_symbol = staticmethod(bits_per_symbol)      # make it a static method.  RTFM

    def _print_verbage(self):
        print "\nDemodulator:"
        print "bits per symbol:     %d"   % self.bits_per_symbol()
        print "Gray code:           %s"   % self._gray_code
        print "RRC roll-off factor: %.2f" % self._excess_bw
        print "Costas Loop alpha:   %.2e" % self._costas_alpha
        print "Costas Loop beta:    %.2e" % self._costas_beta
        print "M&M mu:              %.2f" % self._mm_mu
        print "M&M mu gain:         %.2e" % self._mm_gain_mu
        print "M&M omega:           %.2f" % self._mm_omega
        print "M&M omega gain:      %.2e" % self._mm_gain_omega
        print "M&M omega limit:     %.2f" % self._mm_omega_relative_limit

    def _setup_logging(self):
        print "Modulation logging turned on."
        self.connect(self.pre_scaler,
                     blocks.file_sink(gr.sizeof_gr_complex, "rx_prescaler.dat"))
        self.connect(self.agc,
                     blocks.file_sink(gr.sizeof_gr_complex, "rx_agc.dat"))
        self.connect(self.rrc_filter,
                     blocks.file_sink(gr.sizeof_gr_complex, "rx_rrc_filter.dat"))
        self.connect(self.receiver,
                     blocks.file_sink(gr.sizeof_gr_complex, "rx_receiver.dat"))
        self.connect(self.diffdec,
                     blocks.file_sink(gr.sizeof_gr_complex, "rx_diffdec.dat"))        
        self.connect(self.to_float,
                     blocks.file_sink(gr.sizeof_float, "rx_to_float.dat"))
        self.connect(self.rescale,
                     blocks.file_sink(gr.sizeof_float, "rx_rescale.dat"))

    def add_options(parser):
        """
        Adds modulation-specific options to the standard parser
        """
        parser.add_option("", "--excess-bw", type="float", default=_def_excess_bw,
                          help="set RRC excess bandwith factor [default=%default] (PSK)")
        parser.add_option("", "--no-gray-code", dest="gray_code",
                          action="store_false", default=_def_gray_code,
                          help="disable gray coding on modulated bits (PSK)")
        parser.add_option("", "--costas-alpha", type="float", default=_def_costas_alpha,
                          help="set Costas loop alpha value [default=%default] (PSK)")
        parser.add_option("", "--gain-mu", type="float", default=_def_gain_mu,
                          help="set M&M symbol sync loop gain mu value [default=%default] (PSK)")
        parser.add_option("", "--mu", type="float", default=_def_mu,
                          help="set M&M symbol sync loop mu value [default=%default] (PSK)")
    add_options=staticmethod(add_options)

    def extract_kwargs_from_options(options):
        """
        Given command line options, create dictionary suitable for passing to __init__
        """
        return modulation_utils.extract_kwargs_from_options(
            cqpsk_demod.__init__, ('self',), options)
    extract_kwargs_from_options=staticmethod(extract_kwargs_from_options)


#
# Add these to the mod/demod registry
#
#modulation_utils.add_type_1_mod('cqpsk', cqpsk_mod)
#modulation_utils.add_type_1_demod('cqpsk', cqpsk_demod)

