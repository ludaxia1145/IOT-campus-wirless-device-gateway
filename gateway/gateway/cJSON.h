// cJSON轻量级头文件（简化版）
#ifndef CJSON_H
#define CJSON_H

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)

cJSON *cJSON_New_Item(void);
cJSON *cJSON_CreateObject(void);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateInt(int num);

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string);
void cJSON_AddNumberToObject(cJSON *object, const char *name, double number);

char *cJSON_Print(cJSON *item);
char *cJSON_PrintUnformatted(cJSON *item);
cJSON *cJSON_Parse(const char *value);

void cJSON_Delete(cJSON *c);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int item);

cJSON *cJSON_CreateBool(int boolean);
#endif
