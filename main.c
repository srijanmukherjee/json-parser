#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "json.h"

int main(void) {
    Json *root = json_parse("example.json");
    json_print(root, 4);
    return 0;
}