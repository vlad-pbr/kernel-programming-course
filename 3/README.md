# Assigment 3

Write a kernel module which exposes the following ioctl calls:
- `GET_PHYS_MEM`: receives a buffer with encoded physical frame number, then fills buffer with bytes read from physical memory
- `GET_CR3`: receives buffer, then fills buffer with current value of CR3 register
- `GET_TASK_STRUCT`: receives buffer, then fills buffer with virtual address of current task_struct

Write a C program which runs in user space and utilizes the kernel module in order to read task_struct objects of processes which currently exist and print a process list (similar to `ps` command).

Makefile commands `reinsert` and `clean` take care of kernel module compilation and insertion, `run` command runs the user space program which prints the process list.
