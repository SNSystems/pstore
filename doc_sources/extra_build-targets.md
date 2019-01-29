# Extra build targets

## clang-tidy

If the [clang-tidy](http://clang.llvm.org/extra/clang-tidy/) tool is found on the path, then a number of targets are created which allows to source code for pstore's various libraries and tools to be examined by that tool.

## KLEE

[KLEE](http://klee.github.io) is a symbolic virtual machine built on top of the LLVM compiler infrastructure. It is a powerful tool for exploring the execution paths within code and for discovering hard-to-find bugs that lie buried within.

The pstore cmake project builds a set of targets which use KLEE to generate and execute test cases. These are created if the KLEE tool is available on the path.

### Usage example

The following example uses [Docker](https://docker.com). It must be installed and running before continuing.

Download the KLEE docker image:

    $ docker pull klee/klee
    
Create a container from this image:

    $ docker run --rm -ti --ulimit='stack=-1:-1' -v <path-to-pstore>:/home/klee/pstore klee/klee

The options specified her are:

- The `--rm` option automatically remove the container when it exits.
- The `-ti` options create an interactive command-line propmt.
- The `--ulimit` option sets an unlimited stack size inside the container. This is to avoid stack overflow issues when running KLEE.
- The `-v` switch maps the directory containing the pstore source code on the host into the container as `/home/klee/pstore`.

You will then be presented with a Linux command-line prompt logged into the virtual machine as user "klee". To confirm this:

    klee@ec6d366e9990:~$ whoami
    klee
    klee@ec6d366e9990:~$

Install the build tools (cmake and ninja) into the container:

    klee@ec6d366e9990:~$ wget https://github.com/Kitware/CMake/releases/download/v3.13.1/cmake-3.13.1-Linux-x86_64.sh
    klee@ec6d366e9990:~$ sudo mkdir /opt/cmake
    klee@ec6d366e9990:~$ sudo sh cmake-3.13.1-Linux-x86_64.sh --prefix=/opt/cmake
    klee@ec6d366e9990:~$ export PATH=/opt/cmake/cmake-3.13.1-Linux-x86_64/bin/:$PATH
    klee@ec6d366e9990:~$ sudo apt install ninja-build

    klee@ec6d366e9990:$ cd ~/pstore
    klee@ec6d366e9990:~/pstore$ ./utils/make_build.py -o build_klee -D PSTORE_DISABLE_UINT128_T=Yes
    
Finally, build the project and run the tests:

    klee@ec6d366e9990:~/pstore$ ninja -C build_klee pstore-klee-run-all
