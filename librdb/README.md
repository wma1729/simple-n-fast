# Librdb

Librdb is a simple embeddable database library used to manage millions of key-value pairs. Librdb is thread-safe. Multiple databases can be managed in a single-process. The maximum size of the key and value are 48 and 192 bytes respectively.

### Why another one?

We already have dbm, ndbm, sdbm, qdbm, tokyo cabinet, kyoto cabinet, Berkeley DB, level DB, rock DB. What is need for another one? The reason is simplicity because of the limits imposed on key and value sizes, better control over interfaces (tuned for my needs), and of coarse opportunity to learn.

### Architecture


