#ifndef __LEXER_H__
#define __LEXER_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define LEXER_BUFFER_LENGTH 4096

typedef struct {
    size_t row;
    size_t col;
    const char *filepath;
} Location;

// TODO: handle unicode characters
typedef struct {
    char buf[LEXER_BUFFER_LENGTH];
    size_t offset;
} Buffer;

typedef struct {
    Location location;
    FILE *fp;
    Buffer buffer;
} Lexer;

typedef enum {
    TOK_INVALID = 0,
    TOK_EOF,
    TOK_OBJECT_START, // {
    TOK_OBJECT_END,   // }
    TOK_ARRAY_START,  // [
    TOK_ARRAY_END,    // ]
    TOK_COLON,
    TOK_COMMA,
    TOK_TRUE,
    TOK_NULL,
    TOK_FALSE,
    TOK_STRING,
    TOK_NUMBER_INT,
    TOK_NUMBER_FLOAT
} TokenType;

typedef struct {
    TokenType type;  /* type of token */
    const char *ptr; /* start of lexeme */
    size_t len;      /* length of lexeme */
    Location location;
} Token;

Lexer *lexer_init(const char *filepath);
void lexer_free(Lexer **lexer_ptr);

/* retreives the next token */
Token lexer_get_token(Lexer *lexer);
const char *get_token_name(Token token);

#endif // __LEXER_H__