// cJSON简化实现（仅包含项目需要的功能）
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

static char* cJSON_strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (!copy) return NULL;
    memcpy(copy, str, len);
    return copy;
}

cJSON *cJSON_CreateObject(void) {
    cJSON *item = (cJSON*)calloc(1, sizeof(cJSON));
    if (item) item->type = cJSON_Object;
    return item;
}

cJSON *cJSON_CreateArray(void) {
    cJSON *item = (cJSON*)calloc(1, sizeof(cJSON));
    if (item) item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = (cJSON*)calloc(1, sizeof(cJSON));
    if (item) {
        item->type = cJSON_String;
        item->valuestring = cJSON_strdup(string);
    }
    return item;
}

cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = (cJSON*)calloc(1, sizeof(cJSON));
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

cJSON *cJSON_CreateInt(int num) {
    return cJSON_CreateNumber((double)num);
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (!item) return;
    if (item->string) free(item->string);
    item->string = cJSON_strdup(string);
    if (!object->child) {
        object->child = item;
    } else {
        cJSON *child = object->child;
        while (child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    if (!item) return;
    if (!array->child) {
        array->child = item;
    } else {
        cJSON *child = array->child;
        while (child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
}

static char* print_value(cJSON *item, int depth, int fmt);

static char* print_string(cJSON *item) {
    if (!item || !item->valuestring) return cJSON_strdup("\"\"");
    
    const char *ptr = item->valuestring;
    size_t len = strlen(ptr) + 3;
    for (const char *p = ptr; *p; p++) {
        if (*p == '\"' || *p == '\\' || *p == '\n' || *p == '\r' || *p == '\t') len++;
    }
    
    char *out = (char*)malloc(len);
    if (!out) return NULL;
    
    char *ptr2 = out;
    *ptr2++ = '\"';
    while (*ptr) {
        if (*ptr == '\"' || *ptr == '\\') *ptr2++ = '\\';
        else if (*ptr == '\n') { *ptr2++ = '\\'; *ptr2++ = 'n'; ptr++; continue; }
        else if (*ptr == '\r') { *ptr2++ = '\\'; *ptr2++ = 'r'; ptr++; continue; }
        else if (*ptr == '\t') { *ptr2++ = '\\'; *ptr2++ = 't'; ptr++; continue; }
        *ptr2++ = *ptr++;
    }
    *ptr2++ = '\"';
    *ptr2++ = 0;
    return out;
}

static char* print_number(cJSON *item) {
    char *out = (char*)malloc(64);
    if (!out) return NULL;
    
    if (item->valuedouble == (double)item->valueint) {
        sprintf(out, "%d", item->valueint);
    } else {
        sprintf(out, "%.15g", item->valuedouble);
    }
    return out;
}

static char* print_array(cJSON *item, int depth, int fmt) {
    char **entries;
    char *out = NULL, *ptr, *ret;
    size_t len = 5;
    cJSON *child = item->child;
    int numentries = 0, fail = 0;
    
    while (child) { numentries++; child = child->next; }
    if (!numentries) {
        out = (char*)malloc(3);
        if (out) strcpy(out, "[]");
        return out;
    }
    
    entries = (char**)malloc(numentries * sizeof(char*));
    if (!entries) return NULL;
    memset(entries, 0, numentries * sizeof(char*));
    
    child = item->child;
    for (int i = 0; i < numentries; i++) {
        entries[i] = print_value(child, depth + 1, fmt);
        if (!entries[i]) { fail = 1; break; }
        len += strlen(entries[i]) + 2;
        child = child->next;
    }
    
    if (!fail) out = (char*)malloc(len);
    if (!out) fail = 1;
    
    if (fail) {
        for (int i = 0; i < numentries; i++) if (entries[i]) free(entries[i]);
        free(entries);
        return NULL;
    }
    
    ptr = out;
    *ptr++ = '[';
    for (int i = 0; i < numentries; i++) {
        strcpy(ptr, entries[i]);
        ptr += strlen(entries[i]);
        if (i != numentries - 1) *ptr++ = ',';
        free(entries[i]);
    }
    *ptr++ = ']';
    *ptr++ = 0;
    free(entries);
    return out;
}

static char* print_object(cJSON *item, int depth, int fmt) {
    char **entries = NULL, **names = NULL;
    char *out = NULL, *ptr, *ret, *str;
    size_t len = 7;
    cJSON *child = item->child;
    int numentries = 0, fail = 0;
    
    while (child) { numentries++; child = child->next; }
    if (!numentries) {
        out = (char*)malloc(3);
        if (out) strcpy(out, "{}");
        return out;
    }
    
    entries = (char**)malloc(numentries * sizeof(char*));
    names = (char**)malloc(numentries * sizeof(char*));
    if (!entries || !names) { free(entries); free(names); return NULL; }
    memset(entries, 0, numentries * sizeof(char*));
    memset(names, 0, numentries * sizeof(char*));
    
    child = item->child;
    for (int i = 0; i < numentries; i++) {
        names[i] = str = print_string((cJSON*)&(cJSON){ .valuestring = child->string });
        entries[i] = print_value(child, depth + 1, fmt);
        if (!names[i] || !entries[i]) { fail = 1; break; }
        len += strlen(names[i]) + strlen(entries[i]) + 3;
        child = child->next;
    }
    
    if (!fail) out = (char*)malloc(len);
    if (!out) fail = 1;
    
    if (fail) {
        for (int i = 0; i < numentries; i++) {
            if (names[i]) free(names[i]);
            if (entries[i]) free(entries[i]);
        }
        free(names); free(entries);
        return NULL;
    }
    
    ptr = out;
    *ptr++ = '{';
    for (int i = 0; i < numentries; i++) {
        strcpy(ptr, names[i]); ptr += strlen(names[i]);
        *ptr++ = ':';
        strcpy(ptr, entries[i]); ptr += strlen(entries[i]);
        if (i != numentries - 1) *ptr++ = ',';
        free(names[i]); free(entries[i]);
    }
    *ptr++ = '}';
    *ptr++ = 0;
    free(names); free(entries);
    return out;
}

static char* print_value(cJSON *item, int depth, int fmt) {
    char *out = NULL;
    if (!item) return NULL;
    switch (item->type & 255) {
        case cJSON_NULL:   out = cJSON_strdup("null");  break;
        case cJSON_False:  out = cJSON_strdup("false"); break;
        case cJSON_True:   out = cJSON_strdup("true");  break;
        case cJSON_Number: out = print_number(item); break;
        case cJSON_String: out = print_string(item); break;
        case cJSON_Array:  out = print_array(item, depth, fmt); break;
        case cJSON_Object: out = print_object(item, depth, fmt); break;
    }
    return out;
}

char *cJSON_Print(cJSON *item) {
    return print_value(item, 0, 1);
}

char *cJSON_PrintUnformatted(cJSON *item) {
    return print_value(item, 0, 0);
}

void cJSON_Delete(cJSON *c) {
    cJSON *next;
    while (c) {
        next = c->next;
        if (c->child) cJSON_Delete(c->child);
        if (c->valuestring) free(c->valuestring);
        if (c->string) free(c->string);
        free(c);
        c = next;
    }
}

static const char *parse_skip_whitespace(const char *in) {
    while (in && *in && (*in <= 32)) in++;
    return in;
}

static const char *parse_number(cJSON *item, const char *num) {
    double n = 0, sign = 1, scale = 0;
    int subscale = 0, signsubscale = 1;
    
    if (*num == '-') sign = -1, num++;
    if (*num == '0') num++;
    if (*num >= '1' && *num <= '9') {
        do n = (n * 10.0) + (*num++ - '0');
        while (*num >= '0' && *num <= '9');
    }
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
        num++;
        do n = (n * 10.0) + (*num++ - '0'), scale--;
        while (*num >= '0' && *num <= '9');
    }
    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') num++;
        else if (*num == '-') signsubscale = -1, num++;
        while (*num >= '0' && *num <= '9')
            subscale = (subscale * 10) + (*num++ - '0');
    }
    
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));
    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num;
}

static const char *parse_string(cJSON *item, const char *str) {
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    
    if (*str != '\"') return 0;
    
    while (*ptr != '\"' && *ptr && ++len) {
        if (*ptr++ == '\\') ptr++;
    }
    
    out = (char*)malloc(len + 1);
    if (!out) return 0;
    
    ptr = str + 1;
    ptr2 = out;
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr) {
                case 'b': *ptr2++ = '\b'; break;
                case 'f': *ptr2++ = '\f'; break;
                case 'n': *ptr2++ = '\n'; break;
                case 'r': *ptr2++ = '\r'; break;
                case 't': *ptr2++ = '\t'; break;
                default: *ptr2++ = *ptr; break;
            }
            ptr++;
        }
    }
    *ptr2 = 0;
    if (*ptr == '\"') ptr++;
    item->valuestring = out;
    item->type = cJSON_String;
    return ptr;
}

static const char *parse_value(cJSON *item, const char *value);
static const char *parse_array(cJSON *item, const char *value);
static const char *parse_object(cJSON *item, const char *value);

static const char *parse_value(cJSON *item, const char *value) {
    if (!value) return 0;
    if (!strncmp(value, "null", 4)) {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (!strncmp(value, "false", 5)) {
        item->type = cJSON_False;
        return value + 5;
    }
    if (!strncmp(value, "true", 4)) {
        item->type = cJSON_True;
        item->valueint = 1;
        return value + 4;
    }
    if (*value == '\"') return parse_string(item, value);
    if (*value == '-' || (*value >= '0' && *value <= '9'))
        return parse_number(item, value);
    if (*value == '[') return parse_array(item, value);
    if (*value == '{') return parse_object(item, value);
    return 0;
}

static const char *parse_array(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '[') return 0;
    
    item->type = cJSON_Array;
    value = parse_skip_whitespace(value + 1);
    if (*value == ']') return value + 1;
    
    item->child = child = cJSON_New_Item();
    if (!item->child) return 0;
    value = parse_skip_whitespace(parse_value(child, parse_skip_whitespace(value)));
    if (!value) return 0;
    
    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (!new_item) return 0;
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = parse_skip_whitespace(parse_value(child, parse_skip_whitespace(value + 1)));
        if (!value) return 0;
    }
    
    if (*value == ']') return value + 1;
    return 0;
}

static const char *parse_object(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '{') return 0;
    
    item->type = cJSON_Object;
    value = parse_skip_whitespace(value + 1);
    if (*value == '}') return value + 1;
    
    item->child = child = cJSON_New_Item();
    if (!item->child) return 0;
    value = parse_skip_whitespace(parse_string(child, parse_skip_whitespace(value)));
    if (!value) return 0;
    child->string = child->valuestring;
    child->valuestring = 0;
    if (*value != ':') return 0;
    value = parse_skip_whitespace(parse_value(child, parse_skip_whitespace(value + 1)));
    if (!value) return 0;
    
    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (!new_item) return 0;
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = parse_skip_whitespace(parse_string(child, parse_skip_whitespace(value + 1)));
        if (!value) return 0;
        child->string = child->valuestring;
        child->valuestring = 0;
        if (*value != ':') return 0;
        value = parse_skip_whitespace(parse_value(child, parse_skip_whitespace(value + 1)));
        if (!value) return 0;
    }
    
    if (*value == '}') return value + 1;
    return 0;
}

cJSON *cJSON_Parse(const char *value) {
    cJSON *c = cJSON_New_Item();
    if (!c) return 0;
    
    if (!parse_value(c, parse_skip_whitespace(value))) {
        cJSON_Delete(c);
        return 0;
    }
    return c;
}

cJSON *cJSON_New_Item(void) {
    return (cJSON*)calloc(1, sizeof(cJSON));
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *c = object->child;
    while (c && strcmp(c->string, string)) c = c->next;
    return c;
}

void cJSON_AddStringToObject(cJSON *object, const char *name, const char *string) {
    cJSON *item = cJSON_CreateString(string);
    if (item) {
        cJSON_AddItemToObject(object, name, item);
    }
}

void cJSON_AddNumberToObject(cJSON *object, const char *name, double number) {
    cJSON *item = cJSON_CreateNumber(number);
    if (item) {
        cJSON_AddItemToObject(object, name, item);
    }
}

int cJSON_GetArraySize(const cJSON *array) {
    int size = 0;
    cJSON *c = array->child;
    while (c) {
        size++;
        c = c->next;
    }
    return size;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item) {
    cJSON *c = array->child;
    while (c && item > 0) {
        item--;
        c = c->next;
    }
    return c;
}

cJSON *cJSON_CreateBool(int boolean) {
    cJSON *item = (cJSON*)calloc(1, sizeof(cJSON));
    if (item) {
        item->type = boolean ? cJSON_True : cJSON_False;
        item->valueint = boolean ? 1 : 0;
    }
    return item;
}
