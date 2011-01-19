"""
 (C) 2011 by Holger Hans Peter Freyther
 All Rights Reserved

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import cqpsk

def demod():
    return cqpsk.cqpsk_demod(samples_per_symbol = 2,
                             excess_bw=0.35,
                             costas_alpha=0.03,
                             gain_mu=0.05,
                             mu=0.05,
                             omega_relative_limit=0.05)

