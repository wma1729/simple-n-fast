# libjson

Libjson is a simple JSON library used for JSON data.
- It takes advantage of the C++11 features.
- The caller has to rarely deal with pointers thereby reducing chances of memory leaks.
- UTF-8 support: strings are always stored in memory in their raw, un-escaped form.

### Why another one?

One word: *Simplicity*. And as always an opportunity to learn.

### Library interface

Look at `include/json.h` for reference.

### Examples

Construction of JSON objects and arrays is really straight-forward.

- The following C++ code snippet can be used:
```C++
snf::json::value v_1 = OBJECT {
    KVPAIR("key1", "str_1"),
    KVPAIR("key2", true),
    KVPAIR("key3", false),
    KVPAIR("key4", 456),
    KVPAIR("key5", 1.2e+10),
    KVPAIR("key6", nullptr),
    KVPAIR("key7", ARRAY { "str_2", true, false, 123, 3.14, nullptr })
};
std::cout << v_1.str(true) << std::endl;
```
to generate
```
{
  "key1" : "str_1",
  "key2" : true,
  "key3" : false,
  "key4" : 456,
  "key5" : 1.2e+10,
  "key6" : null,
  "key7" : [
    "str_2",
    true,
    false,
    123,
    3.14,
    null
  ]
}
```

- And this C++ code snippet
```C++
snf::json::value v_1 = ARRAY {
    12345.6,
    true,
    nullptr,
    OBJECT {
        KVPAIR("key1", "val_1"),
        KVPAIR("key2", 6983467)
   }
};
std::cout << v_1.str(true) << std::endl;
```
generates
```
[
  12345.6,
  true,
  null,
  {
    "key1" : "val_1",
    "key2" : 6983467
  }
]
```
