# Simple-N-Fast

Simple-N-Fast is a collection of tools, libraries that I develop in my free time.

* [Java Daemonizer](jdaemon/README.md)

## Compilation
1. Run `configure.sh` on Unix platforms. It should work fine on Linux (and possibly Solaris).
   Run `configure.cmd` on Windows. `configure.[sh|cmd]` generates `Makefile.constants`.
2. Run `make -f Makefile.unix all` on Unix platforms and `nmake /f Makefile.win all` on
   Windows.
