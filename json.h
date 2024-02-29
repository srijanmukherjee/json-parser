#ifndef __JSON_H__
#define __JSON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct Json Json;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_NULL_VALUE
} JsonType;

typedef enum { JSON_NUMBER_INT, JSON_NUMBER_FLOAT } JsonNumberType;

typedef struct {
    const char *key;
    Json *value;
} JsonObjectMember;

typedef struct {
    JsonObjectMember *arr;
    size_t n;
    size_t capacity;
} JsonObject;

typedef struct {
    Json **arr;
    size_t n;
    size_t capacity;
} JsonArray;

typedef const char *JsonString;

// TODO: declare function to convert this to corresponding to int and float
typedef struct {
    JsonNumberType type;
    const char *value;
} JsonNumber;

typedef bool JsonBoolean;

typedef union {
    JsonObject object;
    JsonArray array;
    JsonString string;
    JsonNumber number;
    JsonBoolean boolean;
} JsonValue;

struct Json {
    JsonType type;
    JsonValue value;
};

Json *json_parse(const char *filepath);
void json_print(Json *json, int indent);
void json_fprint(FILE *fp, Json *json, int indent);

#endif // __JSON_H__
