#!/bin/bash

TIMEFORMAT="TIMESPENT: {\"real\": %3R , \"user\": %3U , \"sys\": %3S, \"util\":%P}"
exptime=$(date +"%Y_%m_%d-%H_%M_%S")
srcdir=$(pwd)
statsdir=stats/$exptime
mkdir -p $statsdir
warmup_count=1
iter_count=5

LAUNCHER=build/groundhog-launcher

cmd="$LAUNCHER $iter_count $statsdir"
standalone=0

errors="ERROR\|fail\|fault\|Aborted"

echo "Ensuring the kernel SD-bits tracking is accurate..."
./tests/bin/test_sd_stability


declare -a tests=("hello-world" "hello-world-multithreaded" "simple-operations" "memtest-stack" "memtest-malloc" "memtest-free" "memtest-mmap-none" "memtest-removed" "memtest-madvise"  "memtest-diff" "memtest-multithreaded-malloc")
for test in ${tests[@]};
do
	echo "running $test .."
	$cmd tests/bin/$test > $statsdir/C_$test.out 2>&1
	[[ $(grep $statsdir/C_$test.out -e $errors | wc -l) == "0" ]]  && echo -e "\t\t\tPASSED" || echo -e "\t\t\tFAILED-- not restored"
done

test="simple-operations.py"
echo "running $test .."
$cmd python3 tests/$test > $statsdir/Python_$test.out 2>&1
[[ $(grep $statsdir/Python_$test.out -e $errors | wc -l) == "0" ]]  && echo -e "\t\t\tPASSED" || echo -e "\t\t\tFAILED-- not restored"

exit 0
