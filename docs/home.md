# RTX (Turing) Mesh Shaders {#mainpage}

This is a demo repository for a few techniques made possible by NVIDIA's Turing Architecture (RTX). This repository is used for research and learning purposes. I'm writing a paper on the topic of mesh shaders where I compare the different techniques and determine which techniques would be the most beneficial to implement.

## Building

To build this project you require cmake 3.12 or higher. To make use of mesh shading you need a [RTX capable machine](example.com). You need a C++17 capable compiler. To following compilers are tested:

* [Visual Studio 2019 (16.2.3)](https://visualstudio.microsoft.com/)
* [GCC 9.2](https://gcc.gnu.org/)
* [Clang 8.0.0](https://clang.llvm.org/)

and the following operating systems are tested:

* Windows 10 (version 1809)
* Ubuntu 18.04 LTS
* Arch Linux (Antergos 19.4)

All you need to do to build the project is run these commands in your terminal.

```sh
mkdir build
cd build
cmake -G "[configuration]" ..
```

## Screenshots
