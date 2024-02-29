#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "common.h"

#define TOK(token_type)                                                        \
    (Token) { .type = token_type }

#define TOK_AT(token_type, loc)                                                \
    (Token) { .type = token_type, .location = loc }

static char lexer_read(Lexer *lexer);
static char lexer_current_char(Lexer *lexer);
static Token lexer_get_number(Lexer *lexer);
static Token lexer_get_string(Lexer *lexer);
static inline bool is_whitespace(char c);

// initializes a lexer instance
Lexer *lexer_init(const char *filepath) {
    assert(filepath != NULL);

    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));

    if (lexer == NULL)
        return NULL;

    lexer->fp = fopen(filepath, "r");
    if (lexer->fp == NULL) {
        LOG_ERROR("failed to open file: %s", strerror(errno));
        return NULL;
    }

    lexer->location.row = 1;
    lexer->location.col = 0;
    lexer->location.filepath = filepath;
    // setting offset to LEXER_BUFFER_LENGTH to read data on first call of
    // lexer_read
    lexer->buffer = (Buffer){.buf = {0}, .offset = LEXER_BUFFER_LENGTH};
    return lexer;
}

// frees the lexer and sets it to NULL
void lexer_free(Lexer **lexer_ptr) {
    assert(lexer_ptr != NULL && *lexer_ptr != NULL);
    fclose((*lexer_ptr)->fp);
    free(*lexer_ptr);
    *lexer_ptr = NULL;
}

// returns next token from the input
Token lexer_get_token(Lexer *lexer) {
    assert(lexer != NULL);

    char curr = lexer_read(lexer);

    // skip whitespaces
    while (is_whitespace(curr))
        curr = lexer_read(lexer);

    switch (curr) {
    case '{':
        return TOK_AT(TOK_OBJECT_START, lexer->location);
    case '}':
        return TOK_AT(TOK_OBJECT_END, lexer->location);
    case '[':
        return TOK_AT(TOK_ARRAY_START, lexer->location);
    case ']':
        return TOK_AT(TOK_ARRAY_END, lexer->location);
    case ':':
        return TOK_AT(TOK_COLON, lexer->location);
    case ',':
        return TOK_AT(TOK_COMMA, lexer->location);
    case '"':
        return lexer_get_string(lexer);
    case EOF:
        return TOK_AT(TOK_EOF, lexer->location);
    }

    // tokenize numbers
    if (isdigit(curr) || curr == '-')
        return lexer_get_number(lexer);

    Token token = TOK_AT(TOK_INVALID, lexer->location);

    // tokenize true, false, and null
    NEW_STRING(identifier);

    do {
        APPEND_STRING(identifier, lexer_current_char(lexer));
    } while (isalpha(lexer_read(lexer)));

    if (strcmp(identifier.buf, "true") == 0) {
        token.type = TOK_TRUE;
    } else if (strcmp(identifier.buf, "false") == 0) {
        token.type = TOK_FALSE;
    } else if (strcmp(identifier.buf, "null") == 0) {
        token.type = TOK_NULL;
    } else {
        LOG_ERROR("Invalid token '%s'", identifier.buf);
    }

    lexer->buffer.offset--;

    FREE_STRING(identifier);
    return token;
}

// returns string representation of token
const char *get_token_name(Token token) {
    switch (token.type) {
    case TOK_INVALID:
        return "TOK_INVALID";
    case TOK_OBJECT_START:
        return "TOK_OBJECT_START";
    case TOK_OBJECT_END:
        return "TOK_OBJECT_END";
    case TOK_ARRAY_START:
        return "TOK_ARRAY_START";
    case TOK_ARRAY_END:
        return "TOK_ARRAY_END";
    case TOK_TRUE:
        return "TOK_TRUE";
    case TOK_FALSE:
        return "TOK_FALSE";
    case TOK_NULL:
        return "TOK_NULL";
    case TOK_COLON:
        return "TOK_COLON";
    case TOK_COMMA:
        return "TOK_COMMA";
    case TOK_STRING:
        return "TOK_STRING";
    case TOK_NUMBER_INT:
        return "TOK_NUMBER_INT";
    case TOK_NUMBER_FLOAT:
        return "TOK_NUMBER_FLOAT";
    case TOK_EOF:
        return "TOK_EOF";
    }
    return NULL;
}

static char lexer_read(Lexer *lexer) {
    if (lexer->buffer.offset == LEXER_BUFFER_LENGTH) {
        fread(lexer->buffer.buf, sizeof(char), LEXER_BUFFER_LENGTH, lexer->fp);
        lexer->buffer.offset = 0;
    }

    char c = lexer->buffer.buf[lexer->buffer.offset++];

    if (c == '\n') {
        lexer->location.row++;
        lexer->location.col = -1;
    }

    lexer->location.col++;

    return c;
}

static char lexer_current_char(Lexer *lexer) {
    return lexer->buffer.offset == 0
               ? 0
               : lexer->buffer.buf[lexer->buffer.offset - 1];
}

static inline bool is_string_character(char c) {
    return isalnum(c) || c == '\'' || c == ' ' || ispunct(c);
}

static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v';
}

static Token lexer_get_string(Lexer *lexer) {
    if (lexer_current_char(lexer) != '"')
        return TOK_AT(TOK_INVALID, lexer->location);

    // consume first quote (")
    Location start = lexer->location;
    lexer_read(lexer);

    NEW_STRING(str);
    Token tok = TOK_AT(TOK_STRING, start);

    // TODO: handle other characters
    while (lexer_current_char(lexer) != '"' &&
           is_string_character(lexer_current_char(lexer))) {
        APPEND_STRING(str, lexer_current_char(lexer));
        lexer_read(lexer);
    }

    if (lexer_current_char(lexer) != '"') {
        LOG_ERROR("%s:%zu:%zu: expected \" at the end of string",
                  lexer->location.filepath, lexer->location.row,
                  lexer->location.col);
        tok.type = TOK_INVALID;
        goto defer;
    }

    tok.len = str.n;
    MOVE_STRING_TO_CSTR(str, tok.ptr);

defer:
    FREE_STRING(str);
    return tok;
}

static Token lexer_get_number(Lexer *lexer) {
    Token tok = {.type = TOK_INVALID, .location = lexer->location};
    NEW_STRING(number);

    if (lexer_current_char(lexer) == '-') {
        APPEND_STRING(number, '-');
        lexer_read(lexer);
    }

    int digits = 0;
    while (isdigit(lexer_current_char(lexer))) {
        APPEND_STRING(number, lexer_current_char(lexer));
        lexer_read(lexer);
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    if (lexer_current_char(lexer) == '.') {
        APPEND_STRING(number, '.');
        goto fraction;
    }

    if (tolower(lexer_current_char(lexer)) == 'e') {
        APPEND_STRING(number, lexer_current_char(lexer));
        goto exponent;
    }

    lexer->buffer.offset--;

    tok.type = TOK_NUMBER_INT;
    tok.len = number.n;
    MOVE_STRING_TO_CSTR(number, tok.ptr);
    goto defer;

fraction:
    digits = 0;
    while (isdigit(lexer_read(lexer))) {
        APPEND_STRING(number, lexer_current_char(lexer));
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    if (tolower(lexer_current_char(lexer)) == 'e') {
        APPEND_STRING(number, lexer_current_char(lexer));
        goto exponent;
    }

    lexer->buffer.offset--;

    tok.type = TOK_NUMBER_FLOAT;
    tok.len = number.n;
    MOVE_STRING_TO_CSTR(number, tok.ptr);
    goto defer;

exponent:
    lexer_read(lexer);

    if (lexer_current_char(lexer) == '+' || lexer_current_char(lexer) == '-') {
        APPEND_STRING(number, lexer_current_char(lexer));
        lexer_read(lexer);
    }

    digits = 0;

    while (isdigit(lexer_current_char(lexer))) {
        APPEND_STRING(number, lexer_current_char(lexer));
        lexer_read(lexer);
        digits++;
    }

    if (digits == 0) {
        LOG_ERROR("%s:%zu:%zu: expected digit", lexer->location.filepath,
                  lexer->location.row, lexer->location.col);
        goto defer;
    }

    lexer->buffer.offset--;

    tok.type = TOK_NUMBER_FLOAT;
    tok.len = number.n;
    MOVE_STRING_TO_CSTR(number, tok.ptr);

defer:
    FREE_STRING(number);
    return tok;
}
