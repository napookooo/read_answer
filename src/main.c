#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "cJSON.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#define TrueColor(x) ((x) < 127) ? 255 : 0
#define ColorInRange(x) (x >= 0 && x <= 255)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define Threshold 172

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
  // printf("%i %i ",calculated_value, pass_value);
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

  char *buffer = (char*)malloc(length + 1);
  if (buffer == NULL) {
      printf("Error: Memory allocation failed.\n");
      fclose(fp);
      return NULL;
  }

  size_t read_len = fread(buffer, 1, length, fp);
  buffer[read_len] = '\0';
  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  free(buffer);

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
        // Image_write_color(img, check_x, check_y, width, height, ((i+j)%2 == 0) ? 255 : 0, ans[i * row + j] ? 255 : 0, ((i+j)%2 == 0) ? 0 : 255, 1);
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
        // Image_write_color(img, check_x, check_y, width, height, ((i+j)%2 == 0) ? 255 : 0, ans[i * column + j] ? 255 : 0, ((i+j)%2 == 0) ? 0 : 255, 1);
      }
    }
  }
}

char* OMR_get_chars(bool answers[], char chars[], int column, int row, bool primary){
  char* result;
  if (!primary){
    result = (char*)malloc((column+1) * sizeof(char));

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
    result = (char*)malloc((row+1) * sizeof(char));

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
  for (int i = 0; i < array_length; i++){
    cJSON* choice = cJSON_GetArrayItem(choices, i);
    choice_arr[i] = (char)(choice->valuestring[0]);
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
  bool readID[checkColumn * checkRow];
  OMR_read(img, readID, checkX, checkY, checkColumn, checkRow, width, height, widthNext, heightNext, primary);
  return OMR_get_chars(readID, default_chars, checkColumn, checkRow, primary);
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
  bool readID[checkColumn * checkRow];
  OMR_read(img, readID, checkX, checkY, checkColumn, checkRow, width, height, widthNext, heightNext, primary);
  return OMR_get_chars(readID, default_chars, checkColumn, checkRow, primary);
}

int OMR_get_score(Image* img, cJSON* format, cJSON* subject){
  cJSON* sheet = cJSON_GetObjectItem(format,"Sheet");

  int sheetX = cJSON_GetObjectItem(sheet,"X")->valueint;
  int sheetY = cJSON_GetObjectItem(sheet,"Y")->valueint;
  int sheetColumn = cJSON_GetObjectItem(sheet,"Column")->valueint;
  int sheetRow = cJSON_GetObjectItem(sheet,"Row")->valueint;
  int sheetWidthNext = cJSON_GetObjectItem(sheet,"WidthNext")->valueint;
  int sheetHeightNext = cJSON_GetObjectItem(sheet,"HeightNext")->valueint;
  bool sheetPrimary = cJSON_GetObjectItem(sheet,"Primary")->valueint;

  cJSON* question = cJSON_GetObjectItem(format,"Question");

  int questionWidth = cJSON_GetObjectItem(question,"Width")->valueint;
  int questionHeight = cJSON_GetObjectItem(question,"Height")->valueint;
  int questionColumn = cJSON_GetObjectItem(question,"Column")->valueint;
  int questionRow = cJSON_GetObjectItem(question,"Row")->valueint;
  int questionWidthNext = cJSON_GetObjectItem(question,"WidthNext")->valueint;
  int questionHeightNext = cJSON_GetObjectItem(question,"HeightNext")->valueint;
  bool questionPrimary = cJSON_GetObjectItem(question,"Primary")->valueint;

  int score[sheetColumn * sheetRow];
  cJSON* biases = cJSON_GetObjectItem(subject, "BiasScore");
  int bias;

  char choices[questionPrimary ? questionColumn : questionRow];
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
    bool readAnswer[questionColumn * questionRow];
    OMR_read(img, readAnswer, checkX, checkY, questionColumn, questionRow, questionWidth, questionHeight, questionWidthNext, questionHeightNext, questionPrimary);
    char* charsGet = OMR_get_chars(readAnswer, choices, questionColumn, questionRow, questionPrimary);
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
          Image_write_color(img, checkX+questionWidthNext*(questionPrimary ? index : k), checkY+questionHeightNext*(questionPrimary ? k : index),
          questionWidthNext, questionHeightNext, 0, 255, 0, 0.7f);
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
    // printf("%d. %s\n",i*sheetRow+j+1,charsGet);
  }

  if (current_score > max_score){
    current_score = max_score;
  }

  return current_score;
}

void OMR_get_values(Image* img, cJSON* json_data, int formatID, char** subjectID, char** studentID, int* score){
  cJSON* used_format = OMR_get_format(json_data, formatID);
  *subjectID = OMR_get_subjectID(img, used_format);
  cJSON* used_subject = OMR_get_subject(json_data, *subjectID);
  *studentID = OMR_get_studentID(img, used_format);
  *score = OMR_get_score(img, used_format, used_subject);
}

int main(int argc, char *argv[]) {
  Image test;
  char *file_loc = "./../pict/20250529110230_001.jpg";
  char *save_img = "./pict/test.png";
  // char *save_crop = "./pict/crop1.png";

  Image_load(&test, file_loc);

  // ## Crop 1 ##
  // Image crop1;
  // Image_create(&crop1, 35, 35, test.channels, false);
  // Image_crop(&crop1, &test, 1658, 138);
  Image_greyscale(&test);
  // Image_write_color(&crop1, 0, 0, 700, 670, 0, 255, 0, 0.7f);
  // bool is_black = Image_most_black(&test, 1658, 138, 35, 35, 128);
  // printf("%i\n",is_black);

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

  // Image_free(&grey);
  //   free(visited);
  //   free(contour);

  cJSON* json_data = read_json_file("./src/answer.json");
  int formatID = 0;
  char* subjectID;
  char* studentID;
  int score;
  OMR_get_values(&test, json_data, formatID, &subjectID, &studentID, &score);
  printf("Subject : %s\n",subjectID);
  printf("Student : %s\n", studentID);
  printf("Score : %d\n",score);


  // printf("%s\n", json_str);
  Image_save(&test, save_img);
  // Image_save(&crop1, save_crop);

  Image_free(&test);
  // Image_free(&crop1);
  // char* json_str = cJSON_Print(used_format);
  // cJSON_free(json_str);
  cJSON_Delete(json_data);


  return 0;
}
