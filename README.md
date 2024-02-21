# Doc
[apue - Advanced Programming in the UNIX](https://github.com/Lincheng1993/apue)
[tlpi - The Linux Programming Interface](https://man7.org/tlpi/index.html ; https://man7.org/tlpi/code/online/index.html)

# Build

## tlpi

```sh
    ### [Install third dependency](https://man7.org/tlpi/code/faq.html)
    sudo apt-get install libacl1-dev
    sudo apt-get install libcap-dev
    sudo apt-get install libselinux1-dev

    ### Download the tlpi source code.
    wget "http://man7.org/tlpi/code/download/tlpi-161214-dist.tar.gz"

    ### Unzip the source code, and compile.
    tar -zxvf tlpi-161214-dist.tar.gz
    cd tlpi-dist/
    make -j

    ### If for the existed source code, all already build done,
    ###   we can change code, then 'make -j' to re-build/test


    ### Don't know why need following steps, maybe means we will create our sample base on tlpi.a
        ### Copy the header files to the system include directory.
        cd lib/
        sudo cp tlpi_hdr.h /usr/local/include/
        sudo cp get_num.h /usr/local/include/
        sudo cp error_functions.h /usr/local/include/
        sudo cp ename.c.inc /usr/local/include/

        ### Make a static shared library.
        g++ -c get_num.c error_functions.c
        ar -crv libtlpi.a get_num.o error_functions.o
        sudo cp libtlpi.a /usr/local/lib

        ### How to compile a c file and run it
        g++ <filename> -ltlpi -o <exec_file>
        ./<exec_file>
```

