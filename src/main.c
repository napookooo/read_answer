
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

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

void Image_load(Image *img, const char *fname) {
    if((img->data = stbi_load(fname, &img->width, &img->height, &img->channels, 0)) != NULL) {
        img->size = img->width * img->height * img->channels;
        img->allocation_ = STB_ALLOCATED;
    }
}

void Image_save(const Image *img, const char *fname) {
    stbi_write_jpg(fname, img->width, img->height, img->channels, img->data, 100);
}

void Image_free(Image *img) {
    if(img->allocation_ != NO_ALLOCATION && img->data != NULL) {
        if(img->allocation_ == STB_ALLOCATED) {
            stbi_image_free(img->data);
        } else {
            free(img->data);
        }
        img->data = NULL;
        img->width = 0;
        img->height = 0;
        img->size = 0;
        img->allocation_ = NO_ALLOCATION;
    }
}

void Image_create(Image *img, int width, int height, int channels, bool zeroed) {
    size_t size = width * height * channels;
    if(zeroed) {
        img->data = calloc(size, 1);
    } else {
        img->data = malloc(size);
    }

    if(img->data != NULL) {
        img->width = width;
        img->height = height;
        img->size = size;
        img->channels = channels;
        img->allocation_ = SELF_ALLOCATED;
    }
}

void Image_grayscale(Image* img) {
    if (!img || !img->data || img->channels < 3) return;

    int pixel_count = img->width * img->height;

    for (int i = 0; i < pixel_count; ++i) {
        int idx = i * img->channels;

        uint8_t r = img->data[idx + 0];
        uint8_t g = img->data[idx + 1];
        uint8_t b = img->data[idx + 2];

        uint8_t gray = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);

        img->data[idx + 0] = gray;
        img->data[idx + 1] = gray;
        img->data[idx + 2] = gray;
    }
}

int main(int argc, char *argv[]) {
  Image test;
  char *file_loc = "./../pict/20250529110230_001.jpg";
  char *save_loc = "./pict/test.png";

  Image_load(&test, file_loc);

  Image_grayscale(&test);
  Image_save(&test, save_loc);

  Image_free(&test);

  return 0;
}
