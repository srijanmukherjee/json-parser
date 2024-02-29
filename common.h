#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define LOG_WARN(format, ...)                                                  \
    fprintf(stderr,                                                            \
            ANSI_COLOR_YELLOW "WARN" ANSI_COLOR_RESET ": " format "\n",        \
            ##__VA_ARGS__)

#define LOG_ERROR(format, ...)                                                 \
    fprintf(stderr, ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ": " format "\n",  \
            ##__VA_ARGS__)

#define LOG_INFO(format, ...)                                                  \
    fprintf(stderr, "INFO: " format "\n", ##__VA_ARGS__)

#define STRING_INITIAL_CAPACITY 32

#define NEW_STRING(name)                                                       \
    String name = {                                                            \
        .buf = (char *)malloc(sizeof(char) * (STRING_INITIAL_CAPACITY + 1)),   \
        .capacity = STRING_INITIAL_CAPACITY,                                   \
        .n = 0}

#define APPEND_STRING(str, c)                                                  \
    do {                                                                       \
        if (str.n == str.capacity) {                                           \
            str.capacity += 32;                                                \
            str.buf =                                                          \
                (char *)realloc(str.buf, sizeof(char) * (str.capacity + 1));   \
        }                                                                      \
        str.buf[str.n++] = c;                                                  \
        str.buf[str.n] = 0;                                                    \
    } while (0);

#define FREE_STRING(name)                                                      \
    free(name.buf);                                                            \
    name.buf = NULL;                                                           \
    name.capacity = name.n = 0

#define MOVE_STRING_TO_CSTR(name, dest)                                        \
    dest = (char *)realloc(name.buf, sizeof(char) * (name.n + 1));             \
    name.buf = 0;                                                              \
    name.capacity = 0;                                                         \
    name.n = 0

#define DEFINE_ARRAY(T, var)                                                   \
    typedef struct {                                                           \
        T *arr;                                                                \
        size_t capacity;                                                       \
        size_t n;                                                              \
    } var

#define CREATE_ARRAY(Container, Type, var)                                     \
    Container *var = (Container *)malloc(sizeof(Container));                   \
    if (var == NULL) {                                                         \
        LOG_ERROR("failed to allocate memory for array: %s", strerror(errno)); \
    } else {                                                                   \
        var->capacity = 8;                                                     \
        var->n = 0;                                                            \
        var->arr = (Type *)malloc(sizeof(Type) * var->capacity);               \
        if (var->arr == NULL) {                                                \
            LOG_ERROR("failed to allocate memory for array: %s",               \
                      strerror(errno));                                        \
            free(var);                                                         \
            var = NULL;                                                        \
        }                                                                      \
    }

#define FREE_ARRAY(var)                                                        \
    free(var->arr);                                                            \
    free(var);                                                                 \
    var = NULL;

typedef struct {
    char *buf;
    size_t capacity;
    size_t n;
} String;

#endif // __COMMON_H__