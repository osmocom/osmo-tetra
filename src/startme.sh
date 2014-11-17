#!/bin/bash

 ./demod/python/osmosdr-tetra_demod_fft.py -f 393.0880M -g 10 -o /dev/stdout | ./float_to_bits /dev/stdin /dev/stdout | ./tetra-rx /dev/stdin
