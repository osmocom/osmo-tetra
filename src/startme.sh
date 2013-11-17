#!/bin/bash

 ./demod/python/osmosdr-tetra_demod_fft.py -f 390.912600M -o /dev/stdout | ./float_to_bits /dev/stdin /dev/stdout | ./tetra-rx /dev/stdin
