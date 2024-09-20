FROM ubuntu:18.04
RUN apt-get update
RUN apt update
RUN apt-get install -y --no-install-recommends cmake
RUN apt install -y --no-install-recommends \
  apt-utils vim man less git sudo nano emacs \
  python3 llvm build-essential clang fish zsh ipython3\
  wget curl locales tree zip python3-pip locate \
  gdb valgrind xclip unzip m4 python
# RUN apt-get install -y --no-install-recommends python3-dev

WORKDIR /home/tools
RUN git clone https://github.com/Z3Prover/z3.git
WORKDIR /home/tools/z3
RUN python scripts/mk_make.py
WORKDIR /home/tools/z3/build
RUN make
RUN sudo make install

RUN wget -qO- "https://cmake.org/files/v3.22/cmake-3.22.1-linux-x86_64.tar.gz" | tar --strip-components=1 -xz -C /usr/local

WORKDIR /home/tools
RUN git clone --recursive https://github.com/vlab-cs-ucsb/ABC.git ABC
RUN apt install -y --no-install-recommends autoconf automake libtool intltool flex bison libfl-dev
WORKDIR /home/tools
RUN git clone https://github.com/google/glog.git --branch v0.6.0
WORKDIR /home/tools/glog
RUN cmake -S . -B build -G "Unix Makefiles"
RUN cmake --build build
RUN cmake --build build --target install
WORKDIR /home/tools
RUN git clone https://github.com/cs-au-dk/MONA.git
WORKDIR /home/tools/MONA
RUN git apply --whitespace=nowarn /home/tools/ABC/external/mona/mona_abc.patch
RUN libtoolize && aclocal && automake --gnu --add-missing && autoreconf -ivf
RUN ./configure
RUN make all
RUN sudo make install
RUN sudo ldconfig
WORKDIR /home/tools/ABC
RUN ./autogen.sh
RUN ./configure
RUN make all
RUN sudo make install
RUN sudo ldconfig

WORKDIR /home/tools
RUN wget https://gmplib.org/download/gmp/gmp-6.0.0a.tar.bz2
RUN bzip2 -d gmp-6.0.0a.tar.bz2
RUN tar -xof gmp-6.0.0a.tar
WORKDIR /home/tools/gmp-6.0.0
RUN ./configure
RUN make
RUN make check
RUN sudo make install

WORKDIR /home/tools
RUN wget http://www.shoup.net/ntl/ntl-9.11.0.tar.gz
RUN gunzip ntl-9.11.0.tar.gz
RUN tar -xof ntl-9.11.0.tar
WORKDIR /home/tools/ntl-9.11.0/src
RUN ./configure NTL_GMP_LIP=on GMP_PREFIX=/usr/local/
RUN make 
RUN make check 
RUN sudo make install

#WORKDIR /home/tools
#RUN wget http://barvinok.gforge.inria.fr/barvinok-0.39.tar.gz
#RUN gunzip barvinok-0.39.tar.gz
#COPY barvinok-0.39.tar /home/tools/barvinok-0.39.tar 
#RUN tar -xof barvinok-0.39.tar
#WORKDIR /home/tools/barvinok-0.39
#RUN ./configure NTL_GMP_LIP=on GMP_PREFIX=/usr/local/
#RUN make
#RUN sudo make install

#RUN echo "#! /bin/sh\n/home/tools/barvinok-0.39/iscc < /home/verq2/barvinok_to_run.txt > /home/verq2/barvinok_output.txt" >> /usr/bin/barv_fileread
#RUN chmod +x /usr/bin/barv_fileread

WORKDIR /home/tools
RUN wget https://github.com/latte-int/latte/releases/download/version_1_7_5/latte-integrale-1.7.5.tar.gz
RUN tar -xvf latte-integrale-1.7.5.tar.gz
WORKDIR /home/tools/latte-integrale-1.7.5
RUN ./configure
RUN make

WORKDIR /home/verq2

