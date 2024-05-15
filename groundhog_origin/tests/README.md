# Functional tests description

| Test case     | LastName      |
| ------------- | ------------- |
| hello-world          | A simple test case of printing hello world with a sequence number, restoration resets the sequence numbers |
| simple-operations (c,py)  | Just a longer hello world! |
| memtest-stack | Test call stack restoration |
| memtest-malloc | Allocate memory, then restores |
| memtest-free | Frees allocated memory, then restores |
| memtest-mmap-none | mmap and munmap |
| memtest-removed | Removes a privately mapped memory region, then restores |
| memtest-madvise | Madvises (don't need)  memory pages |
| memtest-diff | Stress test memory modifications: intact, grown maps, shrinked maps, merged maps, unpaged maps, fully mprotected maps, partially mprotected maps | 
| memtest-multithreaded | A multithreaded hello world |
| memtest-multithreaded-malloc | A multithreaded malloc and free test |
