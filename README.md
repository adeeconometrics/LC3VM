# LC3 Assembler

This repo contains C++20 implementation of LC3 Assembler. To build the project, clone the repo and enter the following command in BASH

```Bash
$ cd build && make 
```

To run a program on LC3 emulator, simply include the path of the `*.obj` file in LC3 to the main `LC3VM` program. 

```Bash
$ ./LC3VM [path to pogramfile]
```

The `./Programs` directory contains sample programs obtained from J. Meiners' and R. Pendleton's repo: https://github.com/justinmeiners/lc3-vm.  
