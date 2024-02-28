#ifndef __JSON_H__
#define __JSON_H__

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFINE_ARRAY(T, var)                                                   \
    typedef T T_##var;                                                         \
    typedef struct {                                                           \
        T *arr;                                                                \
        size_t capacity;                                                       \
        size_t n;                                                              \
    } var

typedef struct Json Json;

typedef struct {
    char *buf;
    size_t capacity;
    size_t n;
} String;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_NULL_VALUE
} JsonType;

typedef struct {
    const char *key;
    Json *value;
} JsonKeyValuePair;

DEFINE_ARRAY(JsonKeyValuePair, JsonObject);

DEFINE_ARRAY(Json *, JsonArray);

typedef const char *JsonString;

typedef enum { JSON_NUMBER_INT, JSON_NUMBER_FLOAT } JsonNumberType;

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
void json_dump(Json *root);

///////////////////////////////////////////////////////////////////////////////////////
//                                 IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////

#define JSON_IMPLEMENTATION
#ifdef JSON_IMPLEMENTATION

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define LOG_WARN(format, ...)                                                  \
    fprintf(stderr,                                                            \
            "[" ANSI_COLOR_YELLOW "WARN" ANSI_COLOR_RESET "] " format "\n",    \
            ##__VA_ARGS__)

#define LOG_ERROR(format, ...)                                                 \
    fprintf(stderr,                                                            \
            "[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] " format "\n",      \
            ##__VA_ARGS__)

#define LOG_INFO(format, ...)                                                  \
    fprintf(stderr, "[INFO] " format "\n", ##__VA_ARGS__)

#define JSON_TOKEN(type, ...)                                                  \
    (JsonLexerToken) { type, ##__VA_ARGS__ }

#define STRING_CAPACITY 32

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

#define NEW_STRING(name)                                                       \
    String name = {.buf =                                                      \
                       (char *)malloc(sizeof(char) * (STRING_CAPACITY + 1)),   \
                   .capacity = STRING_CAPACITY,                                \
                   .n = 0}

#define FREE_STRING(name)                                                      \
    free(name.buf);                                                            \
    name.buf = NULL;                                                           \
    name.capacity = name.n = 0

#define MOVE_STRING_TO_CSTR(name, dest)                                        \
    dest = (char *)realloc(name.buf, sizeof(char) * (name.n + 1));             \
    name.buf = 0;                                                              \
    name.capacity = 0;                                                         \
    name.n = 0

typedef struct {
    size_t row;
    size_t col;
    const char *filepath;
} Location;

typedef struct {
    Location location;
    FILE *fp;
    char curr;
} JsonLexer;

typedef enum {
    TOK_ERROR = INT_MIN,
    TOK_EOF,
    TOK_LEFT_BRACE,    // {
    TOK_RIGHT_BRACE,   // }
    TOK_LEFT_BRACKET,  // [
    TOK_RIGHT_BRACKET, // ]
    TOK_BOOLEAN,       // true|false,
    TOK_NULL_VALUE,    // null
    TOK_COLON,         // :
    TOK_COMMA,         // ,
    TOK_STRING_LITERAL,
    TOK_NUMBER_INT,
    TOK_NUMBER_FLOAT,
    TOK_NUMBER_SCIENTIFIC
} JsonTokenType;

const char *represent_json_token_type(JsonTokenType tok) {
    switch (tok) {
    case TOK_ERROR:
        return "TOK_ERROR";
    case TOK_LEFT_BRACE:
        return "TOK_LEFT_BRACE";
    case TOK_RIGHT_BRACE:
        return "TOK_RIGHT_BRACE";
    case TOK_LEFT_BRACKET:
        return "TOK_LEFT_BRACKET";
    case TOK_RIGHT_BRACKET:
        return "TOK_RIGHT_BRACKET";
    case TOK_BOOLEAN:
        return "TOK_BOOLEAN";
    case TOK_NULL_VALUE:
        return "TOK_NULL_VALUE";
    case TOK_COLON:
        return "TOK_COLON";
    case TOK_COMMA:
        return "TOK_COMMA";
    case TOK_STRING_LITERAL:
        return "TOK_STRING_LITERAL";
    case TOK_NUMBER_INT:
        return "TOK_NUMBER_INT";
    case TOK_NUMBER_FLOAT:
        return "TOK_NUMBER_FLOAT";
    case TOK_NUMBER_SCIENTIFIC:
        return "TOK_NUMBER_SCIENTIFIC";
    case TOK_EOF:
        return "TOK_EOF";
    }
    return NULL;
}

typedef struct {
    JsonTokenType type;
    union {
        char *ptr;
        int boolean;
    } value;
} JsonLexerToken;

// TODO: add global error state
typedef struct {
    JsonLexer *lexer;
    JsonLexerToken curr;
} JsonParser;

///////////////////////////////////////////////////////////////////////////////////////
//                                 LEXER IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////

JsonLexer *lexer_init(const char *filepath) {
    assert(filepath != NULL && "[lexer_init] filepath is NULL");

    JsonLexer *lexer = (JsonLexer *)malloc(sizeof(JsonLexer));

    if (lexer == NULL) {
        LOG_WARN("failed to initialize lexer: %s", strerror(errno));
        return NULL;
    }

    lexer->fp = fopen(filepath, "r");
    if (lexer->fp == NULL) {
        LOG_ERROR("failed to open file: %s", strerror(errno));
        return NULL;
    }

    LOG_INFO("opened %s", filepath);

    lexer->location.row = 1;
    lexer->location.col = -1;
    lexer->location.filepath = filepath;
    lexer->curr = ' ';

    return lexer;
}

void lexer_close(JsonLexer **lexer) {
    assert(lexer != NULL && *lexer != NULL &&
           "[lexer_clean] lexer context is NULL");
    fclose((*lexer)->fp);
    free(*lexer);
    *lexer = NULL;
}

char lexer_read_char(JsonLexer *lexer) {
    lexer->curr = fgetc(lexer->fp);
    if (lexer->curr == '\n') {
        lexer->location.row++;
        lexer->location.col = -1;
    }
    lexer->location.col++;
    return lexer->curr;
}

static inline bool is_string_character(char c) {
    return isalnum(c) || c == '\'' || c == ' ' || ispunct(c);
}

static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

JsonLexerToken lexer_get_string(JsonLexer *lexer) {
    if (lexer->curr != '"')
        return JSON_TOKEN(TOK_ERROR);

    // consume first quote (")
    lexer_read_char(lexer);

    NEW_STRING(str);
    JsonLexerToken tok = JSON_TOKEN(TOK_STRING_LITERAL);

    // TODO: handle other characters
    while (lexer->curr != '"' && is_string_character(lexer->curr)) {
        APPEND_STRING(str, lexer->curr);
        lexer_read_char(lexer);
    }

    if (lexer->curr != '"') {
        LOG_ERROR("%s:%zu:%zu: expected \" at the end of string",
                  lexer->location.filepath, lexer->location.row,
                  lexer->location.col);
        tok.type = TOK_ERROR;
        goto defer;
    }

    MOVE_STRING_TO_CSTR(str, tok.value.ptr);

defer:
    FREE_STRING(str);
    return tok;
}

JsonLexerToken lexer_get_number(JsonLexer *lexer) {
    JsonLexerToken tok = {TOK_ERROR};
    NEW_STRING(number);

    if (lexer->curr == '-') {
        APPEND_STRING(number, '-');
        lexer_read_char(lexer);
    }

    int digits = 0;
    while (isdigit(lexer->curr)) {
        APPEND_STRING(number, lexer->curr);
        lexer_read_char(lexer);
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    if (lexer->curr == '.') {
        APPEND_STRING(number, '.');
        goto fraction;
    }

    if (tolower(lexer->curr) == 'e') {
        APPEND_STRING(number, lexer->curr);
        goto exponent;
    }

    tok.type = TOK_NUMBER_INT;
    MOVE_STRING_TO_CSTR(number, tok.value.ptr);
    goto defer;

fraction:
    digits = 0;
    while (isdigit(lexer_read_char(lexer))) {
        APPEND_STRING(number, lexer->curr);
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    if (tolower(lexer->curr) == 'e') {
        APPEND_STRING(number, lexer->curr);
        goto exponent;
    }

    tok.type = TOK_NUMBER_FLOAT;
    MOVE_STRING_TO_CSTR(number, tok.value.ptr);
    goto defer;

exponent:
    lexer_read_char(lexer);

    if (lexer->curr == '+' || lexer->curr == '-') {
        APPEND_STRING(number, lexer->curr);
        lexer_read_char(lexer);
    }

    digits = 0;

    while (isdigit(lexer->curr)) {
        APPEND_STRING(number, lexer->curr);
        lexer_read_char(lexer);
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    tok.type = TOK_NUMBER_SCIENTIFIC;
    MOVE_STRING_TO_CSTR(number, tok.value.ptr);

defer:
    FREE_STRING(number);
    return tok;
}

JsonLexerToken lexer_get_token(JsonLexer *lexer) {
    assert(lexer != NULL && "[lexer_get_token] lexer is NULL");

    lexer_read_char(lexer);

    // skip whitespaces
    while (is_whitespace(lexer->curr))
        lexer_read_char(lexer);

    switch (lexer->curr) {
    case '{':
        return JSON_TOKEN(TOK_LEFT_BRACE);
    case '}':
        return JSON_TOKEN(TOK_RIGHT_BRACE);
    case '[':
        return JSON_TOKEN(TOK_LEFT_BRACKET);
    case ']':
        return JSON_TOKEN(TOK_RIGHT_BRACKET);
    case ':':
        return JSON_TOKEN(TOK_COLON);
    case ',':
        return JSON_TOKEN(TOK_COMMA);
    case '"':
        return lexer_get_string(lexer);
    case EOF:
        return JSON_TOKEN(TOK_EOF);
    }

    // tokenize numbers
    if (isdigit(lexer->curr) || lexer->curr == '-')
        return lexer_get_number(lexer);

    JsonLexerToken token = JSON_TOKEN(TOK_ERROR);

    // tokenize true, false, and null
    NEW_STRING(identifier);

    do {
        APPEND_STRING(identifier, lexer->curr);
    } while (isalpha(lexer_read_char(lexer)));

    if (strcmp(identifier.buf, "true") == 0) {
        token.type = TOK_BOOLEAN;
        token.value.boolean = true;
    } else if (strcmp(identifier.buf, "false") == 0) {
        token.type = TOK_BOOLEAN;
        token.value.boolean = false;
    } else if (strcmp(identifier.buf, "null") == 0) {
        token.type = TOK_NULL_VALUE;
    } else {
        LOG_ERROR("Invalid token '%s'", identifier.buf);
    }

    FREE_STRING(identifier);
    return token;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                   PARSER IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////

JsonParser *parser_init(const char *filepath) {
    JsonLexer *lexer = lexer_init(filepath);
    if (lexer == NULL)
        return NULL;

    JsonParser *parser = (JsonParser *)malloc(sizeof(JsonParser));
    if (parser == NULL) {
        LOG_ERROR("failed to allocated memory for parser: %s", strerror(errno));
        return NULL;
    }

    parser->lexer = lexer;
    return parser;
}

void parser_clean(JsonParser **parser) {
    assert(parser != NULL && *parser != NULL);

    lexer_close(&((*parser)->lexer));
    free(*parser);
    *parser = NULL;
}

JsonLexerToken parser_get_token(JsonParser *parser) {
    assert(parser != NULL);
    return parser->curr = lexer_get_token(parser->lexer);
}

// Defining the grammar
// S -> object | array
// object -> { KVs }
// array -> [ values ]
// KVs -> string: value KVs_temp
// KVs_temp -> , string: value KVs_temp | eps
// values -> value values_temp
// values_temp -> ,values values_temp | eps
// value -> string | number | object | array | bool | null

#define CREATE_ARRAY(Container, var)                                           \
    Container *var = (Container *)malloc(sizeof(Container));                   \
    if (var == NULL) {                                                         \
        LOG_ERROR("failed to allocate memory for array: %s", strerror(errno)); \
    } else {                                                                   \
        var->capacity = 8;                                                     \
        var->n = 0;                                                            \
        var->arr = (T_##Container *)malloc(sizeof(*var->arr) * var->capacity); \
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

void insert_into_object(JsonObject *object, const char *key, Json *value) {
    // check if key already exists
    for (size_t i = 0; i < object->n; i++) {
        if (strcmp(object->arr[i].key, key) == 0) {
            // TODO: override previous value with the new value
            return;
        }
    }

    if (object->n == object->capacity) {
        object->capacity += 16;
        object->arr = (JsonKeyValuePair *)realloc(
            object->arr, sizeof(JsonKeyValuePair) * object->capacity);
    }

    object->arr[object->n++] = (JsonKeyValuePair){.key = key, .value = value};
}

#define RETURN_JSON(node_type, selector, val)                                  \
    {                                                                          \
        Json *json = (Json *)malloc(sizeof(Json));                             \
        json->type = node_type;                                                \
        json->value.selector = val;                                            \
        return json;                                                           \
    }

Json *node_object(JsonParser *parser);
Json *node_array(JsonParser *parser);

Json *node_value(JsonParser *parser) {
    JsonLexerToken token = parser->curr;

    if (token.type == TOK_STRING_LITERAL) {
        RETURN_JSON(JSON_STRING, string, token.value.ptr);
    }

    else if (token.type == TOK_NUMBER_INT || token.type == TOK_NUMBER_FLOAT ||
             token.type == TOK_NUMBER_SCIENTIFIC) {
        JsonNumber value = (JsonNumber){.type = token.type == TOK_NUMBER_INT
                                                    ? JSON_NUMBER_INT
                                                    : JSON_NUMBER_FLOAT,
                                        .value = token.value.ptr};
        RETURN_JSON(JSON_NUMBER, number, value);
    }

    else if (token.type == TOK_BOOLEAN) {
        RETURN_JSON(JSON_BOOLEAN, boolean, token.value.boolean);
    }

    else if (token.type == TOK_NULL_VALUE) {
        Json *json = (Json *)malloc(sizeof(Json));
        json->type = JSON_NULL_VALUE;
        return json;
    } else if (token.type == TOK_LEFT_BRACE) {
        return node_object(parser);
    }

    else if (token.type == TOK_LEFT_BRACKET) {
        return node_array(parser);
    }

    else {
        LOG_ERROR("parser error: invalid token %s",
                  represent_json_token_type(token.type));
    }

    return NULL;
}

Json *node_array(JsonParser *parser) {
    assert(parser->curr.type == TOK_LEFT_BRACKET);

    CREATE_ARRAY(JsonArray, array);
    if (array == NULL)
        return NULL;

    while (true) {
        JsonLexerToken token = parser_get_token(parser);
        if (token.type == TOK_ERROR || token.type == TOK_EOF ||
            token.type == TOK_RIGHT_BRACKET) {
            break;
        }

        if (array->n > 0 && token.type == TOK_COMMA) {
            token = parser_get_token(parser);
        }

        Json *value = node_value(parser);

        if (value == NULL)
            break;

        if (array->n == array->capacity) {
            array->capacity += 16;
            array->arr =
                (Json **)realloc(array->arr, sizeof(Json *) * array->capacity);
        }

        array->arr[array->n++] = value;
    }

    if (parser->curr.type == TOK_ERROR || parser->curr.type == TOK_EOF) {
        FREE_ARRAY(array);
        return NULL;
    }

    Json *json = (Json *)malloc(sizeof(Json));

    if (json == NULL) {
        LOG_ERROR("failed to allocate memory for json node: %s",
                  strerror(errno));
        return NULL;
    }

    json->type = JSON_ARRAY;
    json->value.array = *array;

    return json;
}

Json *node_object(JsonParser *parser) {
    assert(parser->curr.type == TOK_LEFT_BRACE);

    CREATE_ARRAY(JsonObject, object);

    if (object == NULL)
        return NULL;

    while (true) {
        JsonLexerToken token = parser_get_token(parser);
        if (token.type == TOK_ERROR || token.type == TOK_RIGHT_BRACE ||
            token.type == TOK_EOF) {
            break;
        }

        if (object->n > 0 && token.type == TOK_COMMA) {
            token = parser_get_token(parser);
        }

        if (token.type != TOK_STRING_LITERAL) {
            parser->curr.type = TOK_ERROR;
            LOG_ERROR("parsing error: expected key but got %s instead",
                      represent_json_token_type(token.type));
            break;
        }

        const char *key = token.value.ptr;

        token = parser_get_token(parser);
        if (token.type != TOK_COLON) {
            parser->curr.type = TOK_ERROR;
            LOG_ERROR("parsing error: expected colon (:) but got %s instead",
                      represent_json_token_type(token.type));
            break;
        }

        parser_get_token(parser);
        Json *value = node_value(parser);

        insert_into_object(object, key, value);
    }

    if (parser->curr.type == TOK_ERROR || parser->curr.type == TOK_EOF) {
        FREE_ARRAY(object);
        return NULL;
    }

    Json *json = (Json *)malloc(sizeof(Json));

    if (json == NULL) {
        LOG_ERROR("failed to allocate memory for json node: %s",
                  strerror(errno));
        return NULL;
    }

    json->type = JSON_OBJECT;
    json->value.object = *object;

    return json;
}

Json *node_s(JsonParser *parser) {
    JsonLexerToken token = parser_get_token(parser);
    // object
    if (token.type == TOK_LEFT_BRACE) {
        return node_object(parser);
    }

    // array
    if (token.type == TOK_LEFT_BRACKET) {
        return node_array(parser);
    }

    LOG_ERROR("JsonDecodeError: expected object or array at the root");
    return NULL;
}

Json *json_parse(const char *filepath) {
    JsonParser *parser = parser_init(filepath);
    if (parser == NULL)
        return NULL;

    Json *root = node_s(parser);

    parser_clean(&parser);
    return root;
}

void __json_dump(Json *root, int indent) {
    if (root == NULL)
        return;
    switch (root->type) {

    case JSON_OBJECT: {
        printf("{\n");
        for (int i = 0; i < root->value.object.n; i++) {
            if (i > 0)
                printf(",\n");
            printf("%*c", (indent + 4), ' ');
            printf("\"%s\": ", root->value.object.arr[i].key);
            __json_dump(root->value.object.arr[i].value, indent + 4);
        }
        printf("\n%*c}", indent, ' ');
        break;
    }
    case JSON_ARRAY: {
        printf("[\n");
        for (int i = 0; i < root->value.array.n; i++) {
            if (i > 0)
                printf(",\n");
            printf("%*c", (indent + 4), ' ');
            __json_dump(root->value.array.arr[i], indent + 4);
        }
        printf("\n%*c]", indent, ' ');
        break;
    }
    case JSON_STRING: {
        printf("\"%s\"", root->value.string);
        break;
    }
    case JSON_BOOLEAN:
        printf("%s", root->value.boolean ? "true" : "false");
        break;
    case JSON_NUMBER:
        printf("%s", root->value.number.value);
        break;
    case JSON_NULL_VALUE:
        printf("null");
        break;
        break;
    }
}

void json_dump(Json *root) {
    if (root == NULL) {
        return;
    }

    __json_dump(root, 0);
}

#endif // JSON_IMPLEMENTATION
#endif // __JSON_H__
