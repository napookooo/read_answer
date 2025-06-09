#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NOTFILENAME "../pict/20250529110230_001.jpg"
#define IMAGE_CHANNELS 3
#define IMAGE_WIDTH 3507
#define IMAGE_HEIGHT 2480

void load_image(const char *filename, float *image){
  int width, height, channels;
  float *data = stbi_loadf(filename, &width, &height, &channels, IMAGE_CHANNELS);
  stbir_resize_float_linear(data, width, height, 0, image, IMAGE_WIDTH, IMAGE_HEIGHT, 0, STBIR_RGB);
  stbi_image_free(data);
}

int main(int argc, char *argv[]) {
  char* data = stbi_load(NOTFILENAME, x, y, channels_in_file, 3);

  return 0;
}
