#ifndef TYPE_H
#define TYPE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

enum allocation_type {
    NO_ALLOCATION, SELF_ALLOCATED, STB_ALLOCATED
};

typedef struct {
    int width;
    int height;
    int channels;
    size_t size;
    uint8_t *data;
    enum allocation_type allocation_;
} Image;


#endif // !TYPE_H
