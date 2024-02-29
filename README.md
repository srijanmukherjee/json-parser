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

## TODO

- [ ] Error reporting with exact location
- [ ] (IMPORTANT) Handle unicode characters
- [ ] (IMPORTANT) Handle espace sequences in string
- [ ] Implement querying functions
- [ ] Lazy loading (load only queried parts of the tree). Can we possibly load very large json files with this with minimal memory footprint?
- [ ] Tests
- [ ] Benchmark
- [ ] Implement non recursive predictive parsing instead of using top down recursion

## Remarks
Single header was a bad idea. Might revisit in the future.