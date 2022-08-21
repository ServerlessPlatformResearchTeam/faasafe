# Groundhog Source Code
Groundhog's entry file `groundhog-launcher.c` starts the groundhog manager process.
It then forks and execs the function's runtime/program, sets up the communication pipes,
and drops the child process privelages.

`groundhog-launcher.c` expects 3 arguments:
- The number of restorations that will happen
- A directory to output statistics
- The program and its inputs (e.g. python script.py)

`structs.h` has the main datastructures used in storing the child process metadata (`trackedprocess_t`), its memory (`process_mem_info_t`) which points to memory maps (`map_t`), and the individual memory pages (`page_t`).

`proc_control.c` is responsible for interrupting, and resuming the child process. It also opens the child process /proc file system files.

`proc_cr.c` implements the main checkpointing and restoration logic.

`communicator.c` is responsible for intercepting the function's inputs and outputs as well as handling the messages indicating the checkpoint/restore times sent by the optional shim library `c_lib_tracee.c`.

`c_lib_tracee.c` is a simple optional library that gets compiled and can be used as a 
shared library that is included in the child process to communicate the checkpoint/restore
timings with Groundhog. For FaaS, relying on this library doesn't require modification to the tenants functions
because it can be included in the FaaS provider's wrapper that handles the function's inputs and outputs.
Alternatively, the communication of checkpoint/restore times can be inferred by intercepting the functions inputs and
outputs in Groundhog.
