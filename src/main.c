#include "type.h"
#define NOTFILENAME "../pict/20250529110230_001.jpg"
#define IMAGE_CHANNELS 3
#define IMAGE_WIDTH 3507
#define IMAGE_HEIGHT 2480

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Error utility macro
#define ON_ERROR_EXIT(cond, message) \
do { \
    if((cond)) { \
        printf("Error in function: %s at line %d\n", __func__, __LINE__); \
        perror((message)); \
        exit(1); \
    } \
} while(0)

// Check if a string "str" ends with a substring "ends"
static inline bool str_ends_in(const char *str, const char *ends) {
    char *pos = strrchr(str, '.');
    return !strcmp(pos, ends);
}

void Image_load(Image *img, const char *fname);
void Image_free(Image *img);
void Image_save(const Image *img, const char *fname);

int main(int argc, char *argv[]) {

  Image test;

  Image_load(&test, NOTFILENAME);
  Image_save(&test, "test.png");
  Image_free(&test);

  return 0;
}

void Image_load(Image *img, const char *fname) {
    if((img->data = stbi_load(fname, &img->width, &img->height, &img->channels, 0)) != NULL) {
        img->size = img->width * img->height * img->channels;
        img->allocation_ = STB_ALLOCATED;
    }
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

void Image_save(const Image *img, const char *fname) {
    // Check if the file name ends in one of the .jpg/.JPG/.jpeg/.JPEG or .png/.PNG
    if(str_ends_in(fname, ".jpg") || str_ends_in(fname, ".JPG") || str_ends_in(fname, ".jpeg") || str_ends_in(fname, ".JPEG")) {
        stbi_write_jpg(fname, img->width, img->height, img->channels, img->data, 100);
    } else if(str_ends_in(fname, ".png") || str_ends_in(fname, ".PNG")) {
        stbi_write_png(fname, img->width, img->height, img->channels, img->data, img->width * img->channels);
    } else {
        ON_ERROR_EXIT(false, "");
    }
}
