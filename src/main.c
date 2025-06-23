
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

int main(int argc, char *argv[]) {
  Image test;
  char *file_loc = "./../pict/20250529110230_001.jpg";
  char *save_loc1 = "./pict/crop1.png";

  Image_load(&test, file_loc);


  // ## Grey scale ##
  // Image grey;
  // int channels = test.channels == 4 ? 2 : 1;
  // Image_create(&grey, test.width, test.height, channels, false);
  // for(unsigned char *p = test.data, *pg = grey.data; p != test.data + test.size; p += test.channels, pg += grey.channels) {
  //   *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
  //   if(test.channels == 4) {
  //     *(pg + 1) = *(p + 3);
  //   }
  // }

  // ## Crop 1 ##
  Image test2;
  Image_create(&test2, 700, 670, test.channels, false);
  for (int i = 0; i < test2.height; i++) {
    for (int j = 0; j < test2.width; j++) {
      //                                                                   y                      x
      test2.data[(i * test2.width + j) * test.channels + 1] = test.data[((650 + i) * test.width + 10 + j) * test.channels + 1];
      test2.data[(i * test2.width + j) * test.channels + 2] = test.data[((650 + i) * test.width + 10 + j) * test.channels + 2];
      test2.data[(i * test2.width + j) * test.channels + 3] = test.data[((650 + i) * test.width + 10 + j) * test.channels + 3];
    }
  }

  Image_save(&test2, save_loc1);

  Image_free(&test);
  // Image_free(&grey);

  return 0;
}
