#!/bin/bash
# Use this to run the sample trace
# -a Use aligned mode
# -s Number of sets
# -c Bytes per set ( = Number of ways x Line Size)
# -f Tracefile
# Trace File format
# Instruction Count \t R/W \t Instruction Pointer \t Effective Address \t Memory Access Size

bin/ideal -a -s 256 -c 256 -f trace.gz
