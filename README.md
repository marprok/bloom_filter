# bloom_filter
A Bloom filter implementation.

This is a simple and easy to use implementation of a basic [bloom filter](https://en.wikipedia.org/wiki/Bloom_filter).

# Hash Function
The hash function that is used by `bloom_filter` is a template parameter to allow for better customization. 

By default, the hash function is `murmur3` which is in the public domain and can be found [here](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp).

# Usage
The [unit tests](bf_test.cc) that are in this repository can be used as a guide on how to properly use `bloom_filter`.

# Requirements
- cmake: version 3.26.0-rc2 or higher(only in case you want to build the unit tests)
- gcc: 11.4.0 or higher
