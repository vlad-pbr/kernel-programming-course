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
#define GET_PHYS_MEM2 _IOWR(234, 200, unsigned long*)
#define GET_CR3 _IOR(234, 101, unsigned long*)
#define GET_TASK_STRUCT _IOR(234, 102, unsigned long*)

#define GET_CR0 _IOR(234, 110, unsigned long*)
#define GET_CR4 _IOR(234, 111, unsigned long*)

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

    printf("\n");
    
    // iterate each frame number
    for (u_int64_t frame_num = first_frame_num; frame_num <= last_frame_num; frame_num++, frame_buffer_ptr += KM_PAGE_SIZE) {

        printf("asking for frame %ld\n", frame_num);

        printf("bytes: %d %d %d %d %d %d %d %d\n", frame_buffer_ptr[0], frame_buffer_ptr[1], frame_buffer_ptr[2], frame_buffer_ptr[3], frame_buffer_ptr[4], frame_buffer_ptr[5], frame_buffer_ptr[6], frame_buffer_ptr[7] );

        // copy frame number as bytes to buffer
        memcpy(frame_buffer_ptr, (char*)&frame_num, sizeof(frame_num));

        printf("bytes: %d %d %d %d %d %d %d %d\n", frame_buffer_ptr[0], frame_buffer_ptr[1], frame_buffer_ptr[2], frame_buffer_ptr[3], frame_buffer_ptr[4], frame_buffer_ptr[5], frame_buffer_ptr[6], frame_buffer_ptr[7] );

        // request physical memory from module
        ioctl(device_fd, GET_PHYS_MEM, (unsigned long*)frame_buffer_ptr);

        printf("bytes: %d %d %d %d %d %d %d %d\n", frame_buffer_ptr[0], frame_buffer_ptr[1], frame_buffer_ptr[2], frame_buffer_ptr[3], frame_buffer_ptr[4], frame_buffer_ptr[5], frame_buffer_ptr[6], frame_buffer_ptr[7] );
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
    unsigned long pdpt_physical_address;

    // get CR3 register
    ioctl(device_fd, GET_CR3, (unsigned long*)&cr3);
    printf("cr3: %lu\n", cr3);

    // v is a virtual (linear) address
    // in our environment, linear address is 48 bits long
    // therefore only the last 48 bits out of 64 bits are relevant

    // masking out the first 16 bits
    v = v & 0xFFFFFFFFFFFF;
    printf("masked v: %lu\n", v);

    // 4-level paging is enabled so CR3 is comprised as follows
    // [11:0] PCID (used for caching, not relevant)
    // [51:12] physical address of the 4KB-aligned PML4 table (what we need)
    // [64:52] reserved (not relevant)

    // deriving the PML4 table physical address
    pml4_physical_address = (cr3 >> 12) & 0xFFFFFFFFFF; // bit mask with first 40 bits lit
    printf("pml4_physical_address: %lu\n", pml4_physical_address);

    // pml4_physical_address is now the physical address of the
    // 4KB-aligned PML4 table
    // this table comprises 512 64bit entries called PML4Es
    // (PML4 entries)
    // PML4 entry is defined as follows:
    // [51:12] - pml4_physical_address
    // [11:3] - bits [47:39] of linear address
    // [2:0] - set to 0

    // calculate PML4 entry
    pml4e_physical_address = (pml4_physical_address << 12) | ( (v >> 39) << 3 );
    printf("pml4e_physical_address: %lu\n", pml4e_physical_address);

    // copy frame number as bytes to buffer
    // memcpy(frame_buffer_ptr, (char*)&frame_num, sizeof(frame_num));

    // request physical memory from module
    // ioctl(device_fd, GET_PHYS_MEM2, (unsigned long*)&pml4e_physical_address);

    // we can now read the 8 byte PML4 entry from physical memory
    read_phys_mem(pml4e_physical_address, 8, &pdpt_physical_address);
    printf("pdpt_physical_address: %lu\n", pdpt_physical_address);

    // we now have the physical address for the relevant PDPT
    // (page directory pointer table)
    // 

    return 0;

    // // cr3 value is now a physical address of the
    // // 32-byte aligned page directory pointer table
    // // this table comprises four 64bit entries called PDPTEs
    // // (page directory pointer table entries)
    // // bits [31-30] of v are the offset for that table

    // // calculating physical address of PDPT entry
    // printf("pdpt_physical_address: %lu\n", cr3);
    // pdpte_physical_address = cr3 + ((v >> 30) * 8);
    // printf("pdpte_physical_address: %lu\n", pdpte_physical_address);

    // // we need to read the first 8 bytes (64 bits) at this physical address
    // // so we would be able to read the value of the PDPT entry
    // // printf("pdpe bytes: %d %d %d %d %d %d %d %d\n", pdpte_buffer[0], pdpte_buffer[1], pdpte_buffer[2], pdpte_buffer[3], pdpte_buffer[4], pdpte_buffer[5], pdpte_buffer[6], pdpte_buffer[7] );
    // read_phys_mem(pdpte_physical_address, 8, &pdpte);
    // // printf("pdpe bytes: %d %d %d %d %d %d %d %d\n", pdpte_buffer[0], pdpte_buffer[1], pdpte_buffer[2], pdpte_buffer[3], pdpte_buffer[4], pdpte_buffer[5], pdpte_buffer[6], pdpte_buffer[7] );

    // // the very first bit tells us whether the entry actually points to a page directory
    // // let's check that it's lit
    // printf("enabled: %lu\n", pdpte & 1);

    return 0;
}

int main(int argc, char *argv[]) {

    // get ts virtual address from module
    // call v2p                                 <- this is the only problem then
    // call read_phys_mem to read bytes of task struct
    
    // we can access the parent pointer from ts
    // which is already a physical address
    // call read_phys_mem to read bytes of parent task struct
    
    // and so on until we read pid 0
    // we then iterate over its children and children of children
    // and print data about them

    // define variables
    char *device_path;
    unsigned long address;
    unsigned long physical_address;
    unsigned long cr0;
    unsigned long cr4;

    // make device path
    device_path = malloc(6 + strlen(DEVICE_NAME));
    sprintf(device_path, "/dev/%s", DEVICE_NAME);

    // open device
    device_fd = open(device_path, O_RDWR);

    // the entire process
    if (strcmp(argv[1], "103") == 0) {

        // get values of cr0 and cr4 registers
        ioctl(device_fd, GET_CR0, (unsigned long*)&cr0);
        ioctl(device_fd, GET_CR4, (unsigned long*)&cr4);

        // this program assumes it is being ran in a kernel which uses:
        // - protected mode (cr0.pe = 1)
        // - physical address extension (cr4.pae = 1)
        // - 4 level paging (cr4.la57 = 0)
        // - process-context identifiers (cr4.pcide = 1)
        // in other words - 4-level paging

        // print relevant values of register bits
        printf("cr0.pe: %lu\n", cr0 & 1);
        printf("cr4.pae: %lu\n", (cr4 >> 5) & 1);
        printf("cr4.la57: %lu\n", (cr4 >> 12) & 1);
        printf("cr4.pcide: %lu\n", (cr4 >> 17) & 1);

        // get task_struct virtual address
        ioctl(device_fd, GET_TASK_STRUCT, &address);
        printf("task_struct virtual (linear) address: %lu\n", address);

        if (1) {
            // translate virtual address of task_struct to a physical one
            physical_address = v2p(address);
            printf("task_struct physical address: %lu\n", physical_address);
        }
    }

    // close device
    close(device_fd);

}