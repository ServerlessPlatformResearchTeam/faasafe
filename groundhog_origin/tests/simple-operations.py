#File              : register_reload_test.py
#Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
#Date              : 24.04.2020
#Last Modified Date: 24.04.2020
#Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>

import os
from sys import argv, stdout, stdin, exit
from ctypes import *
so_file = "build/groundhog-tracee-lib.so"
groundhog_functions = CDLL(so_file)

def snapshot():
    groundhog_functions.checkpoint_me()

def restore():
    groundhog_functions.restore_me()

def main(params):
    print("Welcome to the memory diff test case\n");
    operations_count = 0
    print("\n=========================================\n");
    print("Will do an operation, checkpoint and then do 2 operations after the checkpoint\n");
    print("Operation %d: 1 + 2 = %d\n" % (operations_count, 1+2))
    operations_count += 1
    snapshot()
    if operations_count != 1:
        print("ERROR")
        exit(-1)

    print("Operation %d: 2 + 3 = %d\n" % (operations_count, 2+3))
    operations_count += 1
    print("Operation %d: 3 + 4 = %d\n"% (operations_count, 3+4))
    operations_count += 1
    print("\n=========================================\n")
    
    restore()

    print("Operation %d: 4 + 5 = %d\n" % (operations_count, 4+5))
    operations_count += 1
    print("Operation %d: 5 + 6 = %d\n" % (operations_count, 5+6))
    operations_count += 1

    print("\n=========================================\n");

if __name__ == '__main__':
    main(0)
