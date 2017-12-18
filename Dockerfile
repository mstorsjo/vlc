FROM llvm-mingw:latest

RUN apt-get update && apt-get install -y gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 mingw-w64-tools lua5.2 libtool automake autoconf autopoint make gettext pkg-config qt4-dev-tools qt5-default git subversion cmake cvs wine64-development-tools zip p7zip nsis bzip2 yasm ragel ant default-jdk protobuf-compiler dos2unix vim gperf

ARG CORES=4

RUN git config --global user.name "VideoLAN Buildbot" && \
    git config --global user.email buildbot@videolan.org

WORKDIR /build/vlc

ADD extras/tools /build/vlc/extras/tools/
RUN cd extras/tools && ./bootstrap && make -j$CORES
ENV PATH=/build/vlc/extras/tools/build/bin:$PATH

ADD contrib /build/vlc/contrib/
RUN mkdir -p contrib/win32 && \
    cd contrib/win32 && \
    ../bootstrap --host=armv7-w64-mingw32 && \
    make -j$CORES fetch
RUN cd contrib/win32 && \
    make -j$CORES -k
ADD . /build/vlc
RUN /build/merge_archives.sh /build/prefix/armv7-w64-mingw32/lib/libmingw32.a /build/prefix/lib/clang/6.0.0/lib/windows/libclang_rt.builtins-arm.a
RUN rm -rf /build/prefix/armv7-w64-mingw32/include/GL
RUN ./bootstrap
ENV PKG_CONFIG_LIBDIR=/build/vlc/contrib/armv7-w64-mingw32/lib/pkgconfig
RUN echo git > src/revision.txt
RUN mkdir win32 && \
    cd win32 && \
    ../extras/package/win32/configure.sh --host=armv7-w64-mingw32 --build=x86_64-pc-linux-gnu && \
    make -j$CORES -k
RUN cd win32 && \
    make package-win-common
