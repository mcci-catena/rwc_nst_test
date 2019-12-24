#!/bin/bash

#
# convert input file (log from 'rw' test) into CSV file for subsequent
# analysis. The columns are:
#
# 1) "target" -- the actual time for starting receive, in micro-seconds
# 2) "nominal" -- the input value to the LMIC scheduler.
# 3) "good" -- number of packets received without error
# 4) "total" -- total number of receive attempts.
#
# File is written to stdout.
#
# If no file is given, reads from standard input. If multiple files are given,
# they are procesed sequentially.
#

awk '
    BEGIN {
        printf("%s,%s,%s,%s\n","target","nominal","good","total");
    } 
    
    /^Window/ {
        nominal = $2; adjusted = $5;
    } 
    
    /^window/ { 
        a = $4; 
        split($4, F, "/"); 
        good = F[1]; 
        total = F[2]; 
        printf("%s,%s,%s,%s\n", adjusted, nominal, good, total); 
    }' "$@"
