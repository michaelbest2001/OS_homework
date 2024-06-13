#include "os.h"
#include <stdio.h>



// Function to update the page table
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    uint64_t index;
    uint64_t pt_addr = pt << 12;
    uint64_t *pt_pointer;
    int level;
    for(level = 0; level <= 4; level++){
        pt_pointer =  (uint64_t *)phys_to_virt(pt_addr);
        index = (vpn >> ((4 - level) * 9)) & 0x1FF;
     
        if(!(pt_pointer[index] & 1)){
            uint64_t new_pt = alloc_page_frame();
            pt_pointer[index] = (new_pt << 12) + 1;
        }
        pt_addr = pt_pointer[index] - 1;
    }
    if (ppn == NO_MAPPING){
        pt_pointer[index] = 0;
        return;
    }
    pt_pointer[index] = (ppn << 12) + 1;
}

// Function to query the page table
uint64_t page_table_query(uint64_t pt, uint64_t vpn){
    uint64_t index;
    uint64_t pt_addr = pt << 12;
    uint64_t *pt_pointer;
    int level;
    for(level = 0; level <= 4; level++){
        pt_pointer =  (uint64_t *)phys_to_virt(pt_addr);

        index = (vpn >> ((4 - level) * 9)) & 0x1FF;
        if(!(pt_pointer[index] & 1)){
            return NO_MAPPING;
        }
        pt_addr = pt_pointer[index] - 1;
    }
    return pt_pointer[index] >> 12;

}
