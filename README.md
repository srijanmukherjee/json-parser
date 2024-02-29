# json parser
Parse and query json files using only a ~~single header file~~ single static library!

## References
- https://www.json.org/json-en.html

## Grammar

```
S -> object | array
object -> { KVs }
array -> [ values ]
KVs -> string: value KVs_temp
KVs_temp -> , string: value KVs_temp | eps
values -> value values_temp
values_temp -> ,values values_temp | eps
value -> string | number | object | array | bool | null
```

## Remarks
Single header was a bad idea. Might revisit in the future.