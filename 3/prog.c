// prog.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE_NAME "km"
#define KM_PAGE_SIZE 4096

// ioctl commands
#define GET_PHYS_MEM _IOWR(234, 100, char*)
#define GET_CR3 _IOR(234, 101, unsigned long*)
#define GET_TASK_STRUCT _IOR(234, 102, unsigned long*)

// task_struct related
#define OFFSET_COMM 2856
#define OFFSET_PID 2384
#define OFFSET_TASKS 2120
#define OFFSET_PREV OFFSET_TASKS + TASK_LEN
#define COMM_LEN 16
#define PID_LEN sizeof(pid_t)
#define TASK_LEN 8

// global variables
static int device_fd;

void read_phys_mem(unsigned long pa, unsigned long len, void *buf) {

    // define variables
    unsigned int first_frame_offset;
    unsigned int first_frame_num;
    unsigned int last_frame_num;
    char *frame_buffer;
    char *frame_buffer_ptr;

    // physical address is not necessarely going to be at the start of the page
    // we need to get the number of the frame which contains it
    first_frame_offset = pa % KM_PAGE_SIZE;
    first_frame_num = (pa - first_frame_offset) / KM_PAGE_SIZE;

    // requested memory will not necessarely reside on the same page
    // we need to get the number of the last frame which contains out data
    last_frame_num = ((pa + len) - ( (pa + len) % KM_PAGE_SIZE )) / KM_PAGE_SIZE;

    // allocate memory for buffer which will contain all of the pages
    frame_buffer = malloc( ( 1 + (first_frame_num - last_frame_num) ) * KM_PAGE_SIZE );
    frame_buffer_ptr = frame_buffer;
    
    // iterate each frame number
    for (u_int64_t frame_num = first_frame_num; frame_num <= last_frame_num; frame_num++, frame_buffer_ptr += KM_PAGE_SIZE) {

        // copy frame number as bytes to buffer
        memcpy(frame_buffer_ptr, (char*)&frame_num, sizeof(frame_num));

        // request physical memory from module
        ioctl(device_fd, GET_PHYS_MEM, (unsigned long*)frame_buffer_ptr);
    }

    // fill buffer with relevant data
    for (unsigned int buf_index = 0; buf_index < len; buf_index++) {
        ((char*)buf)[buf_index] = frame_buffer[first_frame_offset + buf_index];
    }

    free(frame_buffer);
}

unsigned long v2p(unsigned long v) {

    // define variables
    unsigned long cr3;
    unsigned long pml4_physical_address;
    unsigned long pml4e_physical_address;
    unsigned long pml4e_value;
    unsigned long pdpt_physical_address;
    unsigned long pdpte_physical_address;
    unsigned long pdpte_value;
    unsigned long pdt_physical_address;
    unsigned long pde_physical_address;
    unsigned long pde_value;
    unsigned long pt_physical_address;
    unsigned long pte_physical_address;
    unsigned long pte_value;
    unsigned long p_physical_address;
    unsigned long pe_physical_address;
    unsigned long pe_value;

    // get CR3 register
    ioctl(device_fd, GET_CR3, (unsigned long*)&cr3);

    // v is a virtual (linear) address
    // in our environment, linear address is 48 bits long
    // therefore only the last 48 bits out of 64 bits are relevant

    // masking out the first 16 bits
    v = v & 0xFFFFFFFFFFFF;

    // 4-level paging is enabled so CR3 is comprised as follows
    // [11:0] PCID (used for caching, not relevant)
    // [51:12] physical address of the 4KB-aligned PML4 table (what we need)
    // [64:52] reserved (not relevant)

    // deriving the PML4 table physical address
    pml4_physical_address = (cr3 >> 12) & 0xFFFFFFFFFF; // bit mask with first 40 bits lit

    // pml4_physical_address is now the physical address of the
    // 4KB-aligned PML4 table
    // this table comprises 512 64bit entries called PML4Es
    // (PML4 entries)
    // PML4 entry physical address is defined as follows:
    // [51:12] - pml4_physical_address
    // [11:3] - bits [47:39] of linear address
    // [2:0] - set to 0

    // calculate PML4 entry
    pml4e_physical_address = (pml4_physical_address << 12) | ( (v >> 39) << 3 );

    // we can now read the 8 byte PML4 entry from physical memory
    read_phys_mem(pml4e_physical_address, 8, &pml4e_value);

    // we now have the value of the relevant PML4 entry
    // bits [51:12] specify the physical address of the relevant PDPT

    // calculate the PDPT physical address
    pdpt_physical_address = (pml4e_value >> 12) & 0xFFFFFFFFFF;

    // we now have the physical address for the relevant PDPT
    // (page directory pointer table)
    // this table comprises 512 64bit entries called PDPTEs
    // (PDPT entries)
    // PDPT entry physical address is defined as follows:
    // [51:12] - pdpt_physical_address
    // [11:3] - bits [38:30] of linear address
    // [2:0] - set to 0

    // calculate PDPT entry
    pdpte_physical_address = pdpt_physical_address << 12 | ( ( (v >> 30) & 0x1FF ) << 3 );

    // we can now read the 8 byte PDPT entry from physical memory
    read_phys_mem(pdpte_physical_address, 8, &pdpte_value);

    // we now have the value of the relevant PDPT entry
    // usage of PDPT entry varies depending on its PS flag (bit 7)
    // based on our environment, we can safely assume that
    // the flag is set to 0, indicating that we are dealing with a 4KB PDT
    // bits [51:12] specify the physical address of the relevant PDT

    // calculate the PDT physical address
    pdt_physical_address = (pdpte_value >> 12) & 0xFFFFFFFFFF;

    // we now have the physical address for the relevant PDT
    // (page directory table)
    // this table comprises 512 64bit entries called PDEs
    // (PD entries)
    // PD entry physical address is defined as follows:
    // [51:12] - pdt_physical_address
    // [11:3] - bits [29:21] of linear address
    // [2:0] - set to 0

    // calculate PD entry
    pde_physical_address = pdt_physical_address << 12 | ( ( (v >> 21) & 0x1FF ) << 3 );

    // we can now read the 8 byte PD entry from physical memory
    read_phys_mem(pde_physical_address, 8, &pde_value);

    // we now have the value of the relevant PD entry
    // usage of PD entry varies depending on its PS flag (bit 7)
    // based on our environment, we can safely assume that
    // the flag is set to 0, indicating that we are dealing with a 4KB PT
    // bits [51:12] specify the physical address of the relevant PT

    // calculate the PT physical address
    pt_physical_address = (pde_value >> 12) & 0xFFFFFFFFFF;

    // we now have the physical address for the relevant PT
    // (page table)
    // this table comprises 512 64bit entries called PTEs
    // (PT entries)
    // PT entry physical address is defined as follows:
    // [51:12] - pt_physical_address
    // [11:3] - bits [20:12] of linear address
    // [2:0] - set to 0

    // calculate PT entry
    pte_physical_address = pt_physical_address << 12 | ( ( (v >> 12) & 0x1FF ) << 3 );

    // we can now read the 8 byte PT entry from physical memory
    read_phys_mem(pte_physical_address, 8, &pte_value);

    // we now have the value of the relevant PT entry
    // the first bit (P) of the entry specifies whether it is in use
    // based on our environment, we can safely assume that
    // the flag is set to 1, indicating that the PT entry is indeed in use
    // bits [51:12] specify the physical address of the relevant page

    // calculate the page physical address
    p_physical_address = (pte_value >> 12) & 0xFFFFFFFFFF;

    // we now have the physical address for the relevant page
    // we need to read a specific entry in that page at an offset
    // the offset is specified in bits [11:0] of the linear address

    // calculate the physical address of v
    pe_physical_address = p_physical_address << 12 | ( v & 0xFFF );

    return pe_physical_address;
}

int main() {

    // define variables
    char *device_path;
    unsigned long virtual_address;
    unsigned long physical_address;
    char task_comm[COMM_LEN];
    pid_t task_pid;
    unsigned long task_tasks;
    int reached_pid_0;

    // make device path
    device_path = malloc(6 + strlen(DEVICE_NAME));
    sprintf(device_path, "/dev/%s", DEVICE_NAME);

    // open device
    device_fd = open(device_path, O_RDWR);

    // this program assumes it is being ran in a kernel which uses:
    // - protected mode (cr0.pe = 1)
    // - physical address extension (cr4.pae = 1)
    // - 4 level paging (cr4.la57 = 0)
    // - process-context identifiers (cr4.pcide = 1)
    // in other words - 4-level paging

    // get task_struct virtual address
    ioctl(device_fd, GET_TASK_STRUCT, &virtual_address);

    // translate virtual address of task_struct to a physical one
    physical_address = v2p(virtual_address);

    // loop over all previous tasks until reached pid 0
    reached_pid_0 = 0;
    while (!reached_pid_0) {

        // read pid and comm fields of current task_struct
        read_phys_mem(physical_address + OFFSET_PID, PID_LEN, &task_pid);
        read_phys_mem(physical_address + OFFSET_COMM, COMM_LEN, &task_comm);
        printf("%d %s\n", task_pid, task_comm);
        
        // if task pid is not 0
        if (task_pid) {

            // read previous task_struct
            read_phys_mem(physical_address + OFFSET_PREV, 8, &task_tasks);
            task_tasks = v2p(task_tasks);
            physical_address = task_tasks - OFFSET_TASKS;

        } else {
            reached_pid_0 = 1;
        }

    }

    // close device
    close(device_fd);
    free(device_path);
}