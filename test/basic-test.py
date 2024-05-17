import os
import ctypes

path = f'./build/faasafe-shared-lib.so'
lib = ctypes.cdll.LoadLibrary(path)

if __name__ == "__main__":
    print("[python] pid :", os.getpid())
    lib.faasafe_checkpoint()
    print("[python] checkpoint finished")
    lib.faasafe_dump_stats()
    print("[python] dump stats finished")
    # python variable
    a = 1;
    b = 2;
    print("[python] address of global a :", hex(id(a)))
    print("[python] address of local b :", hex(id(b)))
