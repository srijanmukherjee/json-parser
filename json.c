#include "json.h"

#include "common.h"
#include "lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    PARSER_ERROR = -1,
    PARSER_OK,
} ParserState;

typedef struct {
    Lexer *lexer;
    Token curr;
    ParserState state;
} Parser;

Parser *parser_init(const char *filepath) {
    Lexer *lexer = lexer_init(filepath);
    if (lexer == NULL) {
        LOG_ERROR("failed to initialize lexer: %s", strerror(errno));
        return NULL;
    }

    Parser *parser = (Parser *)malloc(sizeof(Parser));
    if (parser == NULL) {
        LOG_ERROR("failed to allocated memory for parser: %s", strerror(errno));
        return NULL;
    }

    parser->lexer = lexer;
    return parser;
}

void parser_clean(Parser **parser) {
    assert(parser != NULL && *parser != NULL);

    lexer_free(&((*parser)->lexer));
    free(*parser);
    *parser = NULL;
}

static Token parser_get_token(Parser *parser) {
    assert(parser != NULL);
    return parser->curr = lexer_get_token(parser->lexer);
}

static void insert_into_object(JsonObject *object, const char *key,
                               Json *value) {
    // check if key already exists
    for (size_t i = 0; i < object->n; i++) {
        if (strcmp(object->arr[i].key, key) == 0) {
            // TODO: override previous value with the new value
            return;
        }
    }

    if (object->n == object->capacity) {
        object->capacity += 16;
        object->arr = (JsonObjectMember *)realloc(
            object->arr, sizeof(JsonObjectMember) * object->capacity);
    }

    object->arr[object->n++] = (JsonObjectMember){.key = key, .value = value};
}

#define RETURN_JSON(node_type, selector, val)                                  \
    {                                                                          \
        Json *json = (Json *)malloc(sizeof(Json));                             \
        json->type = node_type;                                                \
        json->value.selector = val;                                            \
        return json;                                                           \
    }

static Json *node_object(Parser *parser);
static Json *node_array(Parser *parser);

static Json *node_value(Parser *parser) {
    if (parser->state == PARSER_ERROR)
        return NULL;
    Token token = parser->curr;

    if (token.type == TOK_STRING) {
        RETURN_JSON(JSON_STRING, string, token.ptr);
    }

    else if (token.type == TOK_NUMBER_INT || token.type == TOK_NUMBER_FLOAT) {
        JsonNumber value = (JsonNumber){.type = token.type == TOK_NUMBER_INT
                                                    ? JSON_NUMBER_INT
                                                    : JSON_NUMBER_FLOAT,
                                        .value = token.ptr};
        RETURN_JSON(JSON_NUMBER, number, value);
    }

    else if (token.type == TOK_TRUE || token.type == TOK_FALSE) {
        RETURN_JSON(JSON_BOOLEAN, boolean, token.type == TOK_TRUE);
    }

    else if (token.type == TOK_NULL) {
        Json *json = (Json *)malloc(sizeof(Json));
        json->type = JSON_NULL_VALUE;
        return json;
    } else if (token.type == TOK_OBJECT_START) {
        return node_object(parser);
    }

    else if (token.type == TOK_ARRAY_START) {
        return node_array(parser);
    }

    else {
        LOG_ERROR("parser error: invalid token %s", get_token_name(token));
    }

    return NULL;
}

static Json *node_array(Parser *parser) {
    if (parser->state == PARSER_ERROR) {
        return NULL;
    }

    assert(parser->curr.type == TOK_ARRAY_START);

    CREATE_ARRAY(JsonArray, Json *, array);
    if (array == NULL)
        return NULL;

    while (parser->state == PARSER_OK) {
        Token token = parser_get_token(parser);
        if (token.type == TOK_INVALID || token.type == TOK_EOF ||
            token.type == TOK_ARRAY_END) {
            break;
        }

        if (array->n > 0 && token.type != TOK_COMMA) {
            parser->state = PARSER_ERROR;
            LOG_ERROR("%s:%zu:%zu: Expected comma but got %s (\"%s\") instead",
                      token.location.filepath, token.location.row,
                      token.location.col, get_token_name(token), token.ptr);
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

    if (parser->state == PARSER_ERROR) {
        FREE_ARRAY(array);
        return NULL;
    }

    if (parser->curr.type != TOK_ARRAY_END) {
        LOG_ERROR("Missing right bracket");
        parser->state = PARSER_ERROR;
    }

    if (parser->state == PARSER_ERROR || parser->curr.type == TOK_INVALID ||
        parser->curr.type == TOK_EOF) {
        FREE_ARRAY(array);
        return NULL;
    }

    Json *json = (Json *)malloc(sizeof(Json));

    if (json == NULL) {
        LOG_ERROR("failed to allocate memory for json node: %s",
                  strerror(errno));
        parser->state = PARSER_ERROR;
        return NULL;
    }

    json->type = JSON_ARRAY;
    json->value.array = *array;

    return json;
}

static Json *node_object(Parser *parser) {
    if (parser->state == PARSER_ERROR) {
        return NULL;
    }
    assert(parser->curr.type == TOK_OBJECT_START);

    CREATE_ARRAY(JsonObject, JsonObjectMember, object);

    if (object == NULL) {
        LOG_ERROR("Failed to allocate memory for object: %s", strerror(errno));
        parser->state = PARSER_ERROR;
        return NULL;
    }

    while (parser->state == PARSER_OK) {
        Token token = parser_get_token(parser);

        if (token.type == TOK_INVALID || token.type == TOK_OBJECT_END ||
            token.type == TOK_EOF) {
            break;
        }

        // multiple kv pairs must be separated by comma
        if (object->n > 0 && token.type != TOK_COMMA) {
            parser->state = PARSER_ERROR;
            LOG_ERROR("%s:%zu:%zu: Expected comma but got %s (\"%s\") instead",
                      token.location.filepath, token.location.row,
                      token.location.col, get_token_name(token), token.ptr);
            break;
        }

        if (object->n > 0 && token.type == TOK_COMMA) {
            token = parser_get_token(parser);
        }

        if (token.type != TOK_STRING) {
            parser->curr.type = TOK_INVALID;
            LOG_ERROR("parsing error: expected key but got %s (\"%s\") instead",
                      get_token_name(token), token.ptr);
            parser->state = PARSER_ERROR;
            break;
        }

        const char *key = token.ptr;

        token = parser_get_token(parser);
        if (token.type != TOK_COLON) {
            parser->curr.type = TOK_INVALID;
            LOG_ERROR("parsing error: expected colon (:) but got the token %s "
                      "instead",
                      get_token_name(token));
            parser->state = PARSER_ERROR;
            break;
        }

        parser_get_token(parser);
        Json *value = node_value(parser);

        insert_into_object(object, key, value);
    }

    if (parser->state == PARSER_ERROR) {
        FREE_ARRAY(object);
        return NULL;
    }

    if (parser->curr.type != TOK_OBJECT_END) {
        LOG_ERROR("Missing right brace ( } )");
        parser->state = PARSER_ERROR;
    }

    if (parser->state == PARSER_ERROR || parser->curr.type == TOK_INVALID ||
        parser->curr.type == TOK_EOF) {
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

static Json *node_s(Parser *parser) {
    Token token = parser_get_token(parser);
    // object
    if (token.type == TOK_OBJECT_START) {
        return node_object(parser);
    }

    // array
    if (token.type == TOK_ARRAY_START) {
        return node_array(parser);
    }

    LOG_ERROR("JsonDecodeError: expected object or array at the root");
    return NULL;
}

Json *json_parse(const char *filepath) {
    Parser *parser = parser_init(filepath);
    if (parser == NULL)
        return NULL;

    Json *root = node_s(parser);

    parser_clean(&parser);
    return root;
}

void __json_print(Json *root, int current_indent, int indent_step) {
    if (root == NULL)
        return;
    switch (root->type) {

    case JSON_OBJECT: {
        printf("{\n");
        for (int i = 0; i < root->value.object.n; i++) {
            if (i > 0)
                printf(",\n");
            printf("%*c", (current_indent + indent_step), ' ');
            printf("\"%s\": ", root->value.object.arr[i].key);
            __json_print(root->value.object.arr[i].value,
                         current_indent + indent_step, indent_step);
        }
        printf("\n%*c}", current_indent, ' ');
        break;
    }
    case JSON_ARRAY: {
        printf("[\n");
        for (int i = 0; i < root->value.array.n; i++) {
            if (i > 0)
                printf(",\n");
            printf("%*c", (current_indent + indent_step), ' ');
            __json_print(root->value.array.arr[i], current_indent + indent_step,
                         indent_step);
        }
        printf("\n%*c]", current_indent, ' ');
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

void json_print(Json *root, int indent) {
    if (root == NULL) {
        return;
    }

    __json_print(root, 0, indent);
    printf("\n");
}