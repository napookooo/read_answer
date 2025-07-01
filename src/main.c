
#include <fenv.h>
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

#define TrueColor(x) ((x) < 127) ? 255 : 0

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

// fuck me
int inside(const int x, const int y, const int w, const int h){
  return x>=0 && y>=0 && x<w && y<h;
}

typedef struct {
  int x, y;
} Point;

void Contour(Image *img, int *visited, Point *contour, int *contour_len) {
  int direction[8][2] = {{0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}};
  
  for (int y=0; y<img->height; y++){
    for (int x=0; x<img->width; x++){
      if(img->data[y*img->width+x]==255 && !visited[y*img->width+x]){
        int func_x = x, func_y = y;
        int dir_ind = 0;
        int start_x = x, start_y = y;

        do{
          contour[*contour_len].x = func_x;
          contour[*contour_len].y = func_y;
          (*contour_len)++;
          visited[func_y*img->width+func_x] = 1;
          int found = 0;
          for (int i = 0; i < 8; i++) {
            int nd = (dir_ind + i) % 8;
            int nx = x + direction[nd][0];
            int ny = y + direction[nd][1];
            if (inside(nx, ny, img->width, img->height) && img->data[ny*img->width+nx] == 255 && !visited[ny*img->width*nx]) {
              img->data[nx + 0] = 255;
              img->data[nx + 1] = 255;
              img->data[nx + 2] = 0;
              func_x = nx;
              func_y = ny;
              dir_ind = (nd + 6) % 8; // turn left
              found = 1;
              break;
            }
          }
          if (!found) break;
        }while (!(func_x == start_x && func_y == start_y));
      }
    }
  }

}

int main(int argc, char *argv[]) {
  Image test;
  char *file_loc = "./../pict/20250529110230_001.jpg";
  char *save_img = "./pict/test.png";
  char *save_crop = "./pict/crop1.png";

  Image_load(&test, file_loc);


  // ## Grey scale ##
  Image grey;
  int channels = test.channels == 4 ? 2 : 1;
  Image_create(&grey, test.width, test.height, channels, false);
  for(unsigned char *p = test.data, *pg = grey.data; p != test.data + test.size; p += test.channels, pg += grey.channels) {
    *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
    if(test.channels == 4) {
      *(pg + 1) = *(p + 3);
    }
  }
  // ## Inverse color ##
  for (int i = 0; i < grey.width*grey.height; ++i) {
    int idx = i * grey.channels;
    grey.data[idx + 0] = TrueColor(grey.data[idx+0]);
    grey.data[idx + 1] = TrueColor(grey.data[idx+1]);
    grey.data[idx + 2] = TrueColor(grey.data[idx+2]);
  }
  int *visited = calloc(grey.width * grey.height, sizeof(int));
  Point *contour = malloc(grey.width * grey.height * sizeof(Point));
  int contour_len = 0;

  Contour(&grey, visited, contour, &contour_len);

  printf("Found contour with %d points:\n", contour_len);
  for (int i = 0; i < contour_len; i++) {
    printf("(%d, %d)\n", contour[i].x, contour[i].y);
  }


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

  Image_save(&grey, save_img);
  Image_save(&test2, save_crop);

  Image_free(&test);
  Image_free(&grey);

  return 0;
}
