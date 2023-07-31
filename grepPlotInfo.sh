#!/bin/bash

grep -e "minConfidenceToProbe" -e "step1" $1*.txt > roc.txt
grep -e "userCount" -e "average_nanoseconds_of_detector_update" $1*.txt > averageTime.txt
grep -e "minConfidenceToProbe" -e "total_probes_launched" -e "average_probes_per_detector_update" $1*.txt > probesVsThreshold.txt