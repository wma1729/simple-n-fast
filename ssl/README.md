# SSL package

I use LibreSSL package for SSL. OpenSSL or BoringSSL should work as well.

Download the source package.
- `<ssl-src-package>.tar.gz` is the name of the downloaded package.
- `<ssl-src-path>` is the top-level directory of the package.
- `<this-directory>` is **actually** this directory where the package should be installed for build.
- Linux package is installed under `<this-directory>/Linux_x64` and Windows package is installed under `<this-directory>/Windows_x64`.

The following instructions work well for LibreSSL:

### Linux
```sh
# tar zxf <ssl-src-package>.tar.gz
# cd <ssl-src-path>
# configure --enable-nc --prefix=<this-directory>/Linux_x86
# make
# make install
```

### Windows
```cmd
> tar zxf <ssl-src-package>.tar.gz
> cd <ssl-src-path>
> mkdir build-win
> cd build-win
> cmake -DBUILD_SHARED_LIBS=ON -DENABLE_NC=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<this-directory>\Windows_x86 -G"NMake Makefiles" ..
> nmake /f Makefile
> nmake /f Makefile install
```
