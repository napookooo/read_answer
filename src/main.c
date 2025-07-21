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
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
  img->data = zeroed ? calloc(size, 1) : malloc(size);
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

// void Contour(Image *out, Image *img, int *visited, Point *contour, int *contour_len, int mcontour_len) {
//   int direction[8][2] = {{0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}};
//   int ccount = 0;

//   for (int y = 0; y < img->height; y++) {
//     for (int x = 0; x < img->width; x++) {
//       int idx = y * img->width + x;
//       if (img->data[idx * img->channels] == 255 && !visited[idx]) {
//         int func_x = x, func_y = y;
//         // int dir_ind = 0;
//         int start_x = x, start_y = y;
//         int mstep = img->width * img->height;
//         int yesc = 0;

//         do {
//           if (!inside(func_x, func_y, img->width, img->height)) break;
//           if (*contour_len >= mcontour_len) break;

//           contour[*contour_len].x = func_x;
//           contour[*contour_len].y = func_y;
//           (*contour_len)++;
//           visited[func_y * img->width + func_x] = 1;

//           int found = 0;
//           for (int i = 0; i < 8; i++) {
//             int nx = func_x + direction[i][0];
//             int ny = func_y + direction[i][1];
//             int nidx = ny * img->width + nx;

//             if (inside(nx, ny, img->width, img->height) && img->data[nidx * img->channels] == 255 && !visited[nidx]) {
//               out->data[nidx*out->channels + 0] = ccount%2 ? 255 : 0;
//               out->data[nidx*out->channels + 1] = 0;
//               out->data[nidx*out->channels + 2] = ccount%2 ? 0 : 255;
//               func_x = nx;
//               func_y = ny;
//               // dir_ind = (nd + 6) % 8;
//               found = 1;
//               yesc = 1;
//               break;
//             }
//           }

//           if (!found || --mstep <= 0) break;
//         } while (!(func_x == start_x && func_y == start_y));
//         if(yesc){
//           // printf("found contpur\n");
//           ccount++;
//         }
//       }
//     }
//   }
//   printf("count %d\n", ccount);
// }

void Image_crop(Image* cropped, Image* image, int x, int y){
  for (int i = 0; i < cropped->height; i++)
    for (int j = 0; j < cropped->width; j++)
      for (int c = 0; c < image->channels; c++)
        cropped->data[(i * cropped->width + j) * cropped->channels + c] = image->data[((y + i) * image->width + x + j) * image->channels + c];
}

void Image_write_color(Image* img, int x, int y, int width, int height, int r, int g, int b, float intensity){
  int min_x = MIN(img->width, x+width);
  int min_y = MIN(img->height, y+height);
  for (int i = y; i < min_y; i++)
    for (int j = x; j < min_x; j++) {
      if (r >= 0 && r <= 255) img->data[(i * img->width + j) * img->channels] = (char)(r * intensity + img->data[(i * img->width + j) * img->channels] * (1.0f - intensity));
      if (g >= 0 && g <= 255) img->data[(i * img->width + j) * img->channels + 1] = (char)(g * intensity + img->data[(i * img->width + j) * img->channels + 1] * (1.0f - intensity));
      if (b >= 0 && b <= 255) img->data[(i * img->width + j) * img->channels + 2] = (char)(b * intensity + img->data[(i * img->width + j) * img->channels + 2] * (1.0f - intensity));
    }
}

void Image_greyscale(Image* img){
  for (int i = 0; i < img->height; i++)
    for (int j = 0; j < img->width; j++) {
      char r = img->data[(i * img->width + j) * img->channels];
      char g = img->data[(i * img->width + j) * img->channels + 1];
      char b = img->data[(i * img->width + j) * img->channels + 2];
      char gray = (char)(0.299f * r + 0.587f * g + 0.114f * b);
      img->data[(i * img->width + j) * img->channels] = gray;
      img->data[(i * img->width + j) * img->channels + 1] = gray;
      img->data[(i * img->width + j) * img->channels + 2] = gray;
    }
}

bool Image_most_black(Image* img, int x, int y, int width, int height, unsigned char pass_value){
  int area = width * height;
  int shade = 0;
  unsigned char calculated_value = 0;

  int min_x = MIN(img->width, x+width);
  int min_y = MIN(img->height, y+height);
  for (int i = y; i < min_y; i++)
    for (int j = x; j < min_x; j++) {
      shade += img->data[(i * img->width + j) * img->channels];
    }
  calculated_value = shade / area;
  if (calculated_value >= pass_value) return true;
  return false;
}

int main(int argc, char *argv[]) {
  Image test;
  char *file_loc = "./../pict/20250529110230_001.jpg";
  char *save_img = "./pict/test.png";
  char *save_crop = "./pict/crop1.png";

  Image_load(&test, file_loc);

  // ## Crop 1 ##
  Image crop1;
  Image_create(&crop1, 35, 35, test.channels, false);
  Image_crop(&crop1, &test, 1658, 138);
  Image_greyscale(&test);
  Image_write_color(&crop1, 0, 0, 700, 670, -1, 255, -1, 0.7f);
  bool is_black = Image_most_black(&test, 1658, 138, 35, 35, 128);
  printf("%i",is_black);

  // ## Grey scale ##
  // Image grey;
  // int channels = crop1.channels == 4 ? 2 : 1;
  // Image_create(&grey, crop1.width, crop1.height, channels, false);
  // for (unsigned char *p = crop1.data, *pg = grey.data;
  //      p < crop1.data + crop1.size;
  //      p += crop1.channels, pg += channels) {
  //   *pg = (uint8_t)((p[0] + p[1] + p[2]) / 3);
  //   if (channels == 2) pg[1] = p[3];
  // }
  // for (int i = 0; i < grey.width * grey.height; ++i) {
  //   grey.data[i * channels] = TrueColor(grey.data[i * channels]);
  //   if (channels > 1) grey.data[i * channels + 1] = TrueColor(grey.data[i * channels + 1]);
  // }

  //   int mcontour_len = grey.width * grey.height;
  //   int *visited = calloc(mcontour_len, sizeof(int));
  //   Point *contour = malloc(mcontour_len * sizeof(Point));
  //   int contour_len = 0;

  //   Contour(&crop1, &grey, visited, contour, &contour_len, mcontour_len);
  //   printf("contour %d\n", contour_len);

  Image_save(&test, save_img);
  Image_save(&crop1, save_crop);

  Image_free(&test);
  Image_free(&crop1);
  // Image_free(&grey);
  //   free(visited);
  //   free(contour);

  return 0;
}
