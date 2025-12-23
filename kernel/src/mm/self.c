#include <types.h>

#define PROBE_STEP 4096
#define PROBE_TEST 0xABABAB

uintptr_t memory_end;
bool memory_probed = false;

uintptr_t _probe_memory(uintptr_t start_address) {
    volatile uint64_t* current = (volatile uint64_t*)(start_address + PROBE_STEP);
    *current = PROBE_TEST;

    while (*current == PROBE_TEST) {
        current += PROBE_STEP / sizeof(PROBE_TEST);
        *current = PROBE_TEST;
    }

    memory_probed = true;
    return (uintptr_t)current - PROBE_STEP;
}

uintptr_t get_memory_end(void) {
    if (!memory_probed) {
        memory_end = _probe_memory(kernel_get_image_end());
    }
    return memory_end;
}