#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "json.h"

int main(void) {
    Json *root = json_parse("example.json");
    json_dump(root);
    printf("\n");
    return 0;
}