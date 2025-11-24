#pragma once

#include <cstring>

/**
 * CustomAllocator: A fixed-size memory pool allocator
 */
class CustomAllocator {
public:
    /**
     * Constructor: Allocates the entire memory pool upfront
     * @param slotSize - Size in bytes of each slot
     * @param slotCount - Total number of slots to allocate
     */
    CustomAllocator(int slotSize, int slotCount);
    
    /**
     * Destructor: Frees the memory pool
     */
    ~CustomAllocator();

    // Prevent copying
    CustomAllocator(const CustomAllocator&) = delete;
    CustomAllocator& operator=(const CustomAllocator&) = delete;

    /**
     * Allocate a slot from the pool
     * @return Slot ID (index) on success, -1 if pool is full
     */
    int alloc();

    /**
     * Free a previously allocated slot
     * @param id - The slot ID returned from alloc()
     */
    void freeSlot(int id);

    /**
     * Get a raw pointer to the memory at a given slot
     * @param id - The slot ID
     * @return Raw pointer to the slot's memory
     */
    void* getPtr(int id);

    /**
     * Get allocation statistics
     */
    int getUsedCount() const { return usedCount; }
    int getTotalCount() const { return slotCount; }
    float getUsagePercent() const;

private:
    int slotSize;
    int slotCount;
    char* memory;
    bool* used;
    int usedCount; // Efficiently track usage
};