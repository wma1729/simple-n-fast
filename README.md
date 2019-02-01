# Simple-N-Fast

Simple-N-Fast is a collection of tools, libraries that I develop in my free time.

* [Java Daemonizer](jdaemon/README.md) Converts Java applications to Unix daemon/Windows service.
* [librdb](librdb/README.md) A lightweight thread-safe library to manage millions of key/value pairs.
* [libjson](libjson/README.md) JSON library implementation for C++.
* [liblog](liblog/README.md) A thread-safe feature-rich logging library for C++.
* [libnet](libnet/README.md) Network library (including SSL support) for C++.

## Compilation
1. Run `configure.sh` on Linux platforms. Run `configure.cmd` on Windows. `configure.[sh|cmd]` generates `Makefile.constants`.
2. Run `make -f Makefile.unix all` on Unix platforms and `nmake /f Makefile.win all` on Windows.
