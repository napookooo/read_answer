#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#include "cJSON.h"

#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#define TrueColor(x) ((x) < 127) ? 255 : 0
#define ColorInRange(x) (x >= 0 && x <= 255)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int Threshold = 224;
#define AlignmentThreshold 192
#define AlignmentBlackCount 25
#define SkipAlignment 20

bool Debug = false;
bool MallocDebug = false;

#define amalloc(X) my_malloc( X, __FILE__, __LINE__, __FUNCTION__)

void* my_malloc(size_t size, const char *file, int line, const char *func)
{
  if (!MallocDebug)
    return malloc(size);

  // Debug
  void *p = malloc(size);
  printf ("Allocated = %s, %i, %40s, %p[%li]\n", file, line, func, p, size);

  return p;
}

#define afree(X) my_free( X, __FILE__, __LINE__, __FUNCTION__)

void my_free(void* ptr, const char *file, int line, const char *func)
{
  if (!MallocDebug){
    free(ptr);
    return;
  }

  // Debug
  if (!ptr) {
      printf("Attempted to free NULL at %s:%d (%s)\n", file, line, func);
      return;
  }

  printf("Freed     = %s, %d, %40s, %p\n", file, line, func, ptr);

  free(ptr);
}

char default_chars[10] = {'0','1','2','3','4','5','6','7','8','9'};

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
  if((img->data = stbi_load(fname, &img->width, &img->height, &img->channels, 3)) != NULL) {
    img->channels = 3;
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
      afree(img->data);
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
  img->data = zeroed ? calloc(size, 1) : amalloc(size);
  if(img->data != NULL) {
    img->width = width;
    img->height = height;
    img->size = size;
    img->channels = channels;
    img->allocation_ = SELF_ALLOCATED;
  }
}

int inside(const int x, const int y, const int w, const int h){
  return x>=0 && y>=0 && x<w && y<h;
}

typedef struct {
  int x, y;
} Point;

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
  for (int j = x; j < min_x; j++)
  {
    if (ColorInRange(r)) img->data[(i * img->width + j) * img->channels] = (char)(r * intensity + img->data[(i * img->width + j) * img->channels] * (1.0f - intensity));
    if (ColorInRange(g)) img->data[(i * img->width + j) * img->channels + 1] = (char)(g * intensity + img->data[(i * img->width + j) * img->channels + 1] * (1.0f - intensity));
    if (ColorInRange(b)) img->data[(i * img->width + j) * img->channels + 2] = (char)(b * intensity + img->data[(i * img->width + j) * img->channels + 2] * (1.0f - intensity));
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
  float pi = 3.1415927;
  int area = (int)(width * height * pi / 4);
  int shade = 0;
  unsigned int calculated_value = 0;

  int min_x = MIN(img->width, x+width);
  int min_y = MIN(img->height, y+height);
  for (int i = y; i < min_y; i++)
    for (int j = x; j < min_x; j++) {
      shade += img->data[(i * img->width + j) * img->channels];
    }
  calculated_value = shade / area;
  if (calculated_value <= pass_value) return true;
  return false;
}

cJSON* read_json_file(const char* path) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
      printf("Error: Unable to open the file.\n");
      return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long length = ftell(fp);
  rewind(fp);

  char *buffer = (char*)amalloc(length + 1);
  if (buffer == NULL) {
      printf("Error: Memory allocation failed.\n");
      afree(buffer);
      fclose(fp);
      return NULL;
  }

  size_t read_len = fread(buffer, 1, length, fp);
  buffer[read_len] = '\0';
  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  afree(buffer);

  if (json == NULL) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
          printf("Error before: %s\n", error_ptr);
      }
      return NULL;
  }

  return json;
}

cJSON* OMR_get_format(cJSON* parent, int format_id){
  cJSON* formats = cJSON_GetObjectItem(parent,"Formats");
  cJSON* used_format = cJSON_GetArrayItem(formats, format_id);

  return used_format;
}

cJSON* OMR_get_subject(cJSON* parent, const char* subjectID){
  cJSON* subjects = cJSON_GetObjectItem(parent,"Subjects");
  cJSON* used_subject = cJSON_GetObjectItem(subjects, subjectID);

  return used_subject;
}

int index_of_char(const char *arr, char target) {
    int i = 0;
    while (arr[i] != '\0') {
        if (arr[i] == target) return i;
        i++;
    }
    return -1;
}

void OMR_read(Image* img, bool* ans, int x, int y, int column, int row, int width, int height, int widthNext, int heightNext, bool primary){
  if(!primary){
    for (int j = 0; j < row; j++)
    {
      for (int i = 0; i < column; i++)
      {
        int check_x = x + widthNext * i;
        int check_y = y + heightNext * j;
        ans[i * row + j] = Image_most_black(img, check_x, check_y, width, height, Threshold);
        if (Debug)
          Image_write_color(img, check_x, check_y, width, height, ((i+j)%2 == 0) ? 255 : 0, ans[i * row + j] ? 255 : 0, ((i+j)%2 == 0) ? 0 : 255, 1);
      }
    }
  } else {
    for (int i = 0; i < row; i++)
    {
      for (int j = 0; j < column; j++)
      {
        int check_x = x + widthNext * j;
        int check_y = y + heightNext * i;
        ans[i * column + j] = Image_most_black(img, check_x, check_y, width, height, Threshold);
        if (Debug)
          Image_write_color(img, check_x, check_y, width, height, ((i+j)%2 == 0) ? 255 : 0, ans[i * column + j] ? 255 : 0, ((i+j)%2 == 0) ? 0 : 255, 1);
      }
    }
  }
}

char* OMR_get_chars(bool answers[], char chars[], int column, int row, bool primary){
  char* result;
  if (!primary){
    result = (char*)amalloc((column+1) * sizeof(char));

    for (int i = 0; i < column; i++){
      result[i] = ' ';
      for (int j = 0; j < row; j++){
        if (answers[i * row + j]){
          if (result[i] != ' '){
            result[i] = ' ';
            break;
          }
          result[i] = chars[j];
        }
      }
    }
    
    result[column] = '\0';
  } else {
    result = (char*)amalloc((row+1) * sizeof(char));

    for (int i = 0; i < row; i++){
      result[i] = ' ';
      for (int j = 0; j < column; j++){
        if (answers[i * column + j]){
          if (result[i] != ' '){
            result[i] = ' ';
            break;
          }
          result[i] = chars[j];
        }
      }
    }

    result[row] = '\0';
  }

  return result;
}

void OMR_get_choices(cJSON* subject, char* choice_arr, int array_length){
  cJSON* choices = cJSON_GetObjectItem(subject, "Choices");
  if (!choices) {
    printf("Error: 'Choices' array not found in subject.\n");
    return;
  }

  for (int i = 0; i < array_length; i++){
    cJSON* choice = cJSON_GetArrayItem(choices, i);

    if (!cJSON_IsString(choice) || (choice->valuestring == NULL)) {
      choice_arr[i] = '?';
      continue;
    }

    char* choiceStr = choice->valuestring;
    choice_arr[i] = choiceStr[0];
  }
}

char* OMR_get_subjectID(Image* img, cJSON* format){
  cJSON* subjectIDjson = cJSON_GetObjectItem(format, "SubjectIDCheck");
  int checkX = cJSON_GetObjectItem(subjectIDjson, "X")->valueint;
  int checkY = cJSON_GetObjectItem(subjectIDjson, "Y")->valueint;
  int checkRow = cJSON_GetObjectItem(subjectIDjson, "Row")->valueint;
  int checkColumn = cJSON_GetObjectItem(subjectIDjson, "Column")->valueint;
  int width = cJSON_GetObjectItem(subjectIDjson, "Width")->valueint;
  int height = cJSON_GetObjectItem(subjectIDjson, "Height")->valueint;
  int widthNext = cJSON_GetObjectItem(subjectIDjson, "WidthNext")->valueint;
  int heightNext = cJSON_GetObjectItem(subjectIDjson, "HeightNext")->valueint;
  bool primary = cJSON_GetObjectItem(subjectIDjson,"Primary")->valueint;
  bool* readID = amalloc(checkColumn * checkRow * sizeof(bool));
  OMR_read(img, readID, checkX, checkY, checkColumn, checkRow, width, height, widthNext, heightNext, primary);
  char* chars = OMR_get_chars(readID, default_chars, checkColumn, checkRow, primary);
  afree(readID);
  return chars;
}

char* OMR_get_studentID(Image* img, cJSON* format){
  cJSON* studentIDjson = cJSON_GetObjectItem(format, "StudentIDCheck");
  int checkX = cJSON_GetObjectItem(studentIDjson, "X")->valueint;
  int checkY = cJSON_GetObjectItem(studentIDjson, "Y")->valueint;
  int checkRow = cJSON_GetObjectItem(studentIDjson, "Row")->valueint;
  int checkColumn = cJSON_GetObjectItem(studentIDjson, "Column")->valueint;
  int width = cJSON_GetObjectItem(studentIDjson, "Width")->valueint;
  int height = cJSON_GetObjectItem(studentIDjson, "Height")->valueint;
  int widthNext = cJSON_GetObjectItem(studentIDjson, "WidthNext")->valueint;
  int heightNext = cJSON_GetObjectItem(studentIDjson, "HeightNext")->valueint;
  bool primary = cJSON_GetObjectItem(studentIDjson,"Primary")->valueint;
  bool* readID = amalloc(checkColumn * checkRow * sizeof(bool));
  OMR_read(img, readID, checkX, checkY, checkColumn, checkRow, width, height, widthNext, heightNext, primary);
  char* getChar = OMR_get_chars(readID, default_chars, checkColumn, checkRow, primary);
  afree(readID);
  return getChar;
}

int OMR_get_score(Image* img, cJSON* format, cJSON* subject){
  cJSON* sheet = cJSON_GetObjectItem(format,"Sheet");

  int sheetX = cJSON_GetObjectItem(sheet,"X")->valueint;
  int sheetY = cJSON_GetObjectItem(sheet,"Y")->valueint;
  int sheetColumn = cJSON_GetObjectItem(sheet,"Column")->valueint;
  int sheetRow = cJSON_GetObjectItem(sheet,"Row")->valueint;
  int sheetWidthNext = cJSON_GetObjectItem(sheet,"WidthNext")->valueint;
  int sheetHeightNext = cJSON_GetObjectItem(sheet,"HeightNext")->valueint;
  // bool sheetPrimary = cJSON_GetObjectItem(sheet,"Primary")->valueint;

  cJSON* question = cJSON_GetObjectItem(format,"Question");

  int questionWidth = cJSON_GetObjectItem(question,"Width")->valueint;
  int questionHeight = cJSON_GetObjectItem(question,"Height")->valueint;
  int questionColumn = cJSON_GetObjectItem(question,"Column")->valueint;
  int questionRow = cJSON_GetObjectItem(question,"Row")->valueint;
  int questionWidthNext = cJSON_GetObjectItem(question,"WidthNext")->valueint;
  int questionHeightNext = cJSON_GetObjectItem(question,"HeightNext")->valueint;
  bool questionPrimary = cJSON_GetObjectItem(question,"Primary")->valueint;

  cJSON* biases = cJSON_GetObjectItem(subject, "BiasScore");
  int bias;

  char* choices = amalloc((questionPrimary ? questionColumn : questionRow) * sizeof(char));
  OMR_get_choices(subject, choices, questionPrimary ? questionColumn : questionRow);

  cJSON* answers = cJSON_GetObjectItem(subject, "AnswerList");
  int answerCount = cJSON_GetArraySize(answers);
  char* answer;

  int max_score = cJSON_GetObjectItem(subject, "MaxScore")->valueint;
  int current_score = 0;

  for (int i = 0; i < sheetColumn; i++)
  for (int j = 0; j < sheetRow; j++)
  {
    if (i*sheetRow+j >= answerCount) break;
    int checkX = sheetX + sheetWidthNext * i;
    int checkY = sheetY + sheetHeightNext * j;
    bool* readAnswer = amalloc(questionColumn * questionRow * sizeof(bool));
    OMR_read(img, readAnswer, checkX, checkY, questionColumn, questionRow, questionWidth, questionHeight, questionWidthNext, questionHeightNext, questionPrimary);
    char* charsGet = OMR_get_chars(readAnswer, choices, questionColumn, questionRow, questionPrimary);
    afree(readAnswer);
    answer = cJSON_GetArrayItem(answers, i*sheetRow+j)->valuestring;
    if (!strcmp(charsGet, answer)){
      Image_write_color(img, checkX, checkY, questionWidthNext*questionColumn, questionHeightNext*questionRow, 0, 255, 0, 0.7f);
      bias = cJSON_GetArrayItem(biases, i*sheetRow+j)->valueint;
      current_score += bias;
    } else {
      int slen = strlen(charsGet);
      for (int k = 0; k < slen; k++){
        if (charsGet[k] != answer[k]){
          int index = index_of_char(choices, answer[k]);
          if (index != 0){
            Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? 0 : k), checkY+questionHeightNext*(questionPrimary ? k : 0),
            questionWidthNext*(questionPrimary ? index : 1), questionHeightNext*(questionPrimary ? 1 : index), 255, 0, 0, 0.7f);
            
          }
          if (index != -1){
            Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? index : k), checkY+questionHeightNext*(questionPrimary ? k : index),
            questionWidthNext, questionHeightNext, 0, 255, 0, 0.7f);
          }
          if (index != (questionPrimary ? questionColumn : questionRow)){
            Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? index+1 : k), checkY+questionHeightNext*(questionPrimary ? k : index+1),
            questionWidthNext*(questionPrimary ? questionColumn-index-1 : 1), questionHeightNext*(questionPrimary ? 1 : questionRow-index-1), 255, 0, 0, 0.7f);
          }
        } else {
          Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? 0 : k), checkY+questionHeightNext*(questionPrimary ? k : 0),
          questionWidthNext*(questionPrimary ? questionColumn : 1), questionHeightNext*(questionPrimary ? 1 : questionRow), 255, 127, 0, 0.7f);
        }
        // Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? 0 : k), checkY+questionHeightNext*(questionPrimary ? k : 0),
        // questionWidthNext*(questionPrimary ? questionColumn : 1), questionHeightNext*(questionPrimary ? 1 : questionRow), 255, (charsGet[k] == answer[k]) ? 127 : 0 , 0, 0.7f);
      }
    }
    afree(charsGet);
    // printf("%d. %s\n",i*sheetRow+j+1,charsGet);
  }
  afree(choices);

  if (current_score > max_score){
    current_score = max_score;
  }

  return current_score;
}

void OMR_get_alignment(Image* img, int* left, int* right, int* up, int* down) {
  int w = img->width;
  int h = img->height;
  int channels = img->channels;
  uint8_t* data = img->data;

  int blackCount = 0;

  *left = w;
  for (int x = SkipAlignment; x < w; x++) {
    bool found = false;
    for (int y = SkipAlignment; y < h; y++) {
      int idx = (y * w + x) * channels;
      uint8_t gray = data[idx];
      if (gray <= AlignmentThreshold) blackCount++;
      if (blackCount >= AlignmentBlackCount){
        *left = x;
        found = true;
        break;
      }
    }
    if (found) break;
  }

  blackCount = 0;
  *right = w;
  for (int x = w - SkipAlignment; x >= 0; x--) {
    bool found = false;
    for (int y = SkipAlignment; y < h; y++) {
      int idx = (y * w + x) * channels;
      uint8_t gray = data[idx];
      if (gray <= AlignmentThreshold) blackCount++;
      if (blackCount >= AlignmentBlackCount){
        *right = w - 1 - x;
        found = true;
        break;
      }
    }
    if (found) break;
  }

  blackCount = 0;
  *up = h;
  for (int y = SkipAlignment; y < h; y++) {
    bool found = false;
    for (int x = SkipAlignment; x < w; x++) {
      int idx = (y * w + x) * channels;
      uint8_t gray = data[idx];
      if (gray <= AlignmentThreshold) blackCount++;
      if (blackCount >= AlignmentBlackCount){
        *up = y;
        found = true;
        break;
      }
    }
    if (found) break;
  }

  blackCount = 0;
  *down = h;
  for (int y = h - SkipAlignment; y >= 0; y--) {
    bool found = false;
    for (int x = SkipAlignment; x < w; x++) {
      int idx = (y * w + x) * channels;
      uint8_t gray = data[idx];
      if (gray <= AlignmentThreshold) blackCount++;
      if (blackCount >= AlignmentBlackCount){
        *down = h - 1 - y;
        found = true;
        break;
      }
    }
    if (found) break;
  }

  if (*left == w || *right == w || *up == h || *down == h) {
    *left = *right = *up = *down = -1;
  }

  printf("%d %d %d %d\n", *left, *right, *up, *down);
}

void OMR_image_crop(Image* img) {
  int left, right, up, down;
  OMR_get_alignment(img, &left, &right, &up, &down);

  if (left < 0 || right < 0 || up < 0 || down < 0) {
    printf("Warning: No content found to crop.\n");
    return;
  }

  int new_width = img->width - left - right;
  int new_height = img->height - up - down;

  if (new_width <= 0 || new_height <= 0) {
    printf("Error: Invalid crop dimensions.\n");
    return;
  }

  Image cropped;
  Image_create(&cropped, new_width, new_height, img->channels, false);
  Image_crop(&cropped, img, left, up);

  Image_free(img);
  *img = cropped;
}

void OMR_set_image_size(Image* img, cJSON* paper) {
  int target_w = cJSON_GetObjectItem(paper, "Width")->valueint;
  int target_h = cJSON_GetObjectItem(paper, "Height")->valueint;
  int channels = img->channels;

  if (img->width == target_w && img->height == target_h) {
    return;
  }

  uint8_t* resized_data = (uint8_t*)amalloc(target_w * target_h * channels);
  if (!resized_data) {
    fprintf(stderr, "Failed to allocate resized image.\n");
    return;
  }

  // Resize using stb_image_resize
  stbir_resize_uint8_linear(img->data, img->width, img->height, 0,
                     resized_data, target_w, target_h, 0,
                     channels);

  // Free old data
  afree(img->data);  // or stbi_image_free if STB_ALLOCATED

  img->data = resized_data;
  img->width = target_w;
  img->height = target_h;
  img->size = target_w * target_h * channels;
  img->allocation_ = SELF_ALLOCATED;
}

cJSON* OMR_get_paper(cJSON* format){
  return cJSON_GetObjectItem(format, "Paper");
}

void OMR_get_values(Image* img, cJSON* json_data, int formatID, char** subjectID, char** studentID, int* score){
  OMR_image_crop(img);
  cJSON* used_format = OMR_get_format(json_data, formatID);
  if (!used_format){
    printf("Unknown format ID %d\n", formatID);
    *subjectID = "-1";
    *studentID = "-1";
    *score = -2;
    return;
  }
  cJSON* paper = OMR_get_paper(used_format);
  if (!paper){
    printf("Unknown paper %s\n", used_format);
    *subjectID = "-1";
    *studentID = "-1";
    *score = -3;
    return;
  }
  OMR_set_image_size(img, paper);
  *subjectID = OMR_get_subjectID(img, used_format);
  cJSON* used_subject = OMR_get_subject(json_data, *subjectID);
  if (!used_subject){
    printf("Unknown subject %s\n", subjectID);
    *studentID = "-1";
    *score = -4;
    return;
  }
  *studentID = OMR_get_studentID(img, used_format);
  if (!used_subject){
    printf("Unknown subject ID %s\n", *subjectID);
    *score = -5;
    return;
  }
  *score = OMR_get_score(img, used_format, used_subject);
}

#define MAX_FILES 100

bool is_path(const char *str) {
    return strchr(str, '/') != NULL || strchr(str, '\\') != NULL;
}

bool is_image_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    return strcasecmp(ext, ".jpg") == 0 ||
           strcasecmp(ext, ".jpeg") == 0 ||
           strcasecmp(ext, ".png") == 0;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s [image names or paths] [-d input_dir] [--debug] [--mdebug] [-t Threshold] -f format_file -o output_dir\n", argv[0]);
    return 1;
  }

  char *input_dir = NULL;
  char *format_file = NULL;
  char *output_dir = NULL;
  char *image_paths[MAX_FILES];
  int image_count = 0;
  bool has_dash_d = false;

  // Parse args
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      input_dir = argv[++i];
      has_dash_d = true;
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_dir = argv[++i];
    } else if (argv[i][0] != '-') {
      image_paths[image_count++] = argv[i];
    } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      format_file = argv[++i];
    } else if (strcmp(argv[i], "--debug") == 0) {
      Debug = true;
    } else if (strcmp(argv[i], "--mdebug") == 0) {
      MallocDebug = true;
    } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
      Threshold = atoi(argv[++i]);
    }
  }

  if (!format_file){
    fprintf(stderr, "Error: Format file (-f) is required.\n");
    return 2;
  }

  if (!output_dir) {
    fprintf(stderr, "Error: Output directory (-o) is required.\n");
    return 1;
  }

  // If -d is used, override image_paths[] by scanning input_dir
  if (has_dash_d) {
    DIR *dir = opendir(input_dir);
    if (!dir) {
      perror("Failed to open input directory");
      return 3;
    }

    struct dirent *entry;
    image_count = 0;

    while ((entry = readdir(dir)) != NULL && image_count < MAX_FILES) {
      if (is_image_file(entry->d_name)) {
        image_paths[image_count++] = strdup(entry->d_name); // Save filename only
      }
    }

    closedir(dir);

    if (image_count == 0) {
      fprintf(stderr, "No valid image files found in directory: %s\n", input_dir);
      return 4;
    }
  } else {
    // If not using -d, check that no input path is a directory
    for (int i = 0; i < image_count; i++) {
      if (has_dash_d && is_path(image_paths[i])) {
        fprintf(stderr, "Error: File path '%s' cannot include directories when using -d.\n", image_paths[i]);
        return 5;
      }
    }
  }

  // Open CSV
  char csv_path[512];
  snprintf(csv_path, sizeof(csv_path), "%s/results.csv", output_dir);
  FILE *csv = fopen(csv_path, "w");
  if (!csv) {
    perror("Failed to create CSV file");
    return 6;
  }
  fprintf(csv, "StudentID,SubjectID,Score\n");

  // Load config
  cJSON *json_data = read_json_file(format_file);

  if (!json_data) {
    fprintf(stderr, "Failed to parse format file.\n");
    return 7;
  }

  int formatID = 0;

  for (int i = 0; i < image_count; i++) {
    char full_path[512];
    if (has_dash_d) {
      snprintf(full_path, sizeof(full_path), "%s/%s", input_dir, image_paths[i]);
    } else {
      strncpy(full_path, image_paths[i], sizeof(full_path));
    }

    Image img;
    Image_load(&img, full_path);
    if (img.data == NULL) {
      fprintf(stderr, "Failed to load image: %s\n", full_path);
      continue;
    }

    Image_greyscale(&img);

    char *subjectID = NULL, *studentID = NULL;
    int score = 0;
    OMR_get_values(&img, json_data, formatID, &subjectID, &studentID, &score);

    printf("Image path: %s\nStudent: %s | Subject: %s | Score: %d\n", full_path, studentID, subjectID, score);
    fprintf(csv, "%s,\"%s\",%d\n", studentID, subjectID, score);

    // Save image
    char name_no_ext[256];
    const char *filename = strrchr(image_paths[i], '/');
    filename = filename ? filename + 1 : image_paths[i];
    strncpy(name_no_ext, filename, sizeof(name_no_ext));
    char *dot = strrchr(name_no_ext, '.');
    if (dot) *dot = '\0';

    char output_image_path[512];
    snprintf(output_image_path, sizeof(output_image_path), "%s/%s.jpg", output_dir, name_no_ext);
    Image_save(&img, output_image_path);

    Image_free(&img);
    afree(subjectID);
    afree(studentID);

    if (has_dash_d) {
      free(image_paths[i]); // Because strdup was used
    }
  }

  fclose(csv);
  cJSON_Delete(json_data);
  return 0;
}
