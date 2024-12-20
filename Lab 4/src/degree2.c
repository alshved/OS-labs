#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16  // Минимальный размер блока

int log2s(int n) {
    if (n == 0) {
        return -1;
    }
    int result = 0;
    while (n > 1) {
        n >>= 1;
        result++;
    }
    return result;
}

typedef struct BlockHeader {
    struct BlockHeader *next;
} BlockHeader;

// Структура аллокатора
typedef struct Allocator {
    BlockHeader **free_lists;
    size_t num_lists;
    void *base_addr;
    size_t total_size;
} Allocator;

Allocator *allocator_create(void *memory, size_t size) {
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }
    Allocator *allocator = (Allocator *)memory;
    allocator->base_addr = memory;
    allocator->total_size = size;

    size_t min_usable_size = sizeof(BlockHeader) + MIN_BLOCK_SIZE;

    size_t max_block_size = (size < 32) ? 32 : size;

    allocator->num_lists = (size_t)floor(log2s(max_block_size) / 2) + 3;
    allocator->free_lists =
        (BlockHeader **)((char *)memory + sizeof(Allocator));

    for (size_t i = 0; i < allocator->num_lists; i++) {
        allocator->free_lists[i] = NULL;
    }

    void *current_block = (char *)memory + sizeof(Allocator) +
                          allocator->num_lists * sizeof(BlockHeader *);
    size_t remaining_size =
        size - sizeof(Allocator) - allocator->num_lists * sizeof(BlockHeader *);

    size_t block_size = MIN_BLOCK_SIZE;
    while (remaining_size >= min_usable_size) {
        if (block_size > remaining_size) {
            break;
        }

        if (block_size > max_block_size) {
            break;
        }

        if (remaining_size >= (block_size + sizeof(BlockHeader)) * 2) {
            for (int i = 0; i < 2; i++) {
                BlockHeader *header = (BlockHeader *)current_block;
                size_t index = (size == 0) ? 0 : (size_t)log2s(block_size);
                header->next = allocator->free_lists[index];
                allocator->free_lists[index] = header;

                current_block = (char *)current_block + block_size;
                remaining_size -= block_size;
            }
        } else {
            BlockHeader *header = (BlockHeader *)current_block;
            size_t index = (size == 0) ? 0 : (size_t)log2s(block_size);
            header->next = allocator->free_lists[index];
            allocator->free_lists[index] = header;

            current_block = (char *)current_block + remaining_size;
            remaining_size = 0;
        }

        block_size <<= 1;
    }
    return allocator;
}

// Функция выделения памяти
void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size_t index = (size == 0) ? 0 : log2s(size) + 1;
    if (index >= allocator->num_lists) {
        index = allocator->num_lists;
    }
    bool flag = false;
    if (allocator->free_lists[index] == NULL) {
        while (index <= allocator->num_lists) {
            if (allocator->free_lists[index] != NULL) {
                flag = true;
                break;
            } else {
                ++index;
            }
        }
        if (!flag) return NULL;
    }

    BlockHeader *block = allocator->free_lists[index];
    allocator->free_lists[index] = block->next;

    return (void *)((char *)block + sizeof(BlockHeader));
}

// Функция освобождения памяти
void allocator_free(Allocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return;
    }

    BlockHeader *block = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    size_t temp_size =
        (char *)block + sizeof(BlockHeader) - (char *)allocator->base_addr;
    size_t temp = 32;

    while (temp <= temp_size) {
        size_t next_size = temp << 1;
        if (next_size > temp_size) {
            break;
        }
        temp = next_size;
    }

    size_t index = (temp_size == 0) ? 0 : (size_t)log2s(temp);
    if (index >= allocator->num_lists) {
        index = allocator->num_lists - 1;
    }

    block->next = allocator->free_lists[index];
    allocator->free_lists[index] = block;
}

// Функция уничтожения аллокатора
void allocator_destroy(Allocator *allocator) {
    if (allocator) {
        munmap(allocator->base_addr, allocator->total_size);
    }
}