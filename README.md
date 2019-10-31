<a href="#"><img src="https://i.imgur.com/r6gKEvC.png"></a>

<p align="center">
  <a href="#"><img alt="Release" src="https://img.shields.io/github/release/VZout/RTX-Mesh-Shaders?style=for-the-badge"></a>
  <a href="#"><img alt="Issues" src="https://img.shields.io/github/issues/VZout/RTX-Mesh-Shaders?style=for-the-badge"></a>
  <a href="#"><img alt="License" src="https://img.shields.io/github/license/VZout/RTX-Mesh-Shaders?style=for-the-badge"></a>
  <a href="#"><img alt="Stars" src="https://img.shields.io/github/stars/VZout/RTX-Mesh-Shaders?style=for-the-badge"></a>
</p>

<p align="center">
  <strong>
    Visit <a aria-label="vzout.com/meshshading" href="http://vzout.com/meshshading">vzout.com/meshshading</a> to read the research paper.
  </strong>
</p>

---

**This repository demo's mesh shaders introduced by NVIDIA's Turing Architecture and is used for research purposes. To accompany this project I've written a research paper on the topic of mesh shaders where I compare traditional techniques with their mesh shading equivalent.**

---

## Features

* Mesh Shading Pipeline
* Cook-Torrance Microfacet Specular BRDF
* Burley Diffuse BRDF
* Clear Coat Materials
* Anisotropic Materials
* Approximated Subsurface Scattering Materials
* Deferred Rendering
* Normal and Ambient Occlusion Mapping
* High Dynamic Range Tonemapping
* Image-Based Lighting

## Building

To build this project you require [**CMake 3.15**](https://cmake.org/) or higher and a **C++20 capable compiler**. To run the application you need a GPU and driver that **supports Vulkan 1.1**. To run the application and make use of mesh shaders you need a **[RTX capable gpu](https://en.wikipedia.org/wiki/Nvidia_RTX)**.

* [Visual Studio 2019 (16.2.3)](https://visualstudio.microsoft.com/)
* [GCC 9.2](https://gcc.gnu.org/)
* [Clang 8.0.0](https://clang.llvm.org/)

and the following operating systems are tested:

* Windows 10 (version 1903)
* Ubuntu (18.04 LTS)
* Arch Linux (Antergos 19.4)

To generate the project files run the following command in your terminal:

```sh
mkdir build
cd build
cmake -G "[configuration]" ..
```

## Screenshots

<img src="http://upload.vzout.com/wispvk/img0.png" width="80%"><br>
<img src="https://i.imgur.com/ezUFj1A.png" width="300"> 
<img src="https://i.imgur.com/hyiK8kp.png" width="300"> 

## [License (GNU Affero General Public License 3)](https://opensource.org/licenses/AGPL-3.0)

<a href="https://opensource.org/licenses/AGPL-3.0" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

```
Copyright 2019-2020 Viktor Zoutman

The GNU Affero General Public License is a free, copyleft license for software and other kinds of works,
specifically designed to ensure cooperation with the community in the case of network server software.

The licenses for most software and other practical works are designed to take away your freedom to share
and change the works. By contrast, our General Public Licenses are intended to guarantee your freedom to
share and change all versions of a program--to make sure it remains free software for all its users.

When we speak of free software, we are referring to freedom, not price. Our General Public Licenses are
designed to make sure that you have the freedom to distribute copies of free software (and charge for
them if you wish), that you receive source code or can get it if you want it, that you can change the
software or use pieces of it in new free programs, and that you know you can do these things.

Developers that use our General Public Licenses protect your rights with two steps: (1) assert copyright
on the software, and (2) offer you this License which gives you legal permission to copy, distribute and/or
modify the software.

A secondary benefit of defending all users' freedom is that improvements made in alternate versions of the
program, if they receive widespread use, become available for other developers to incorporate. Many developers
of free software are heartened and encouraged by the resulting cooperation. However, in the case of software
used on network servers, this result may fail to come about. The GNU General Public License permits making a
modified version and letting the public access it on a server without ever releasing its source code to the public.

The GNU Affero General Public License is designed specifically to ensure that, in such cases, the modified
source code becomes available to the community. It requires the operator of a network server to provide the
source code of the modified version running there to the users of that server. Therefore, public use of a modified
version, on a publicly accessible server, gives the public access to the source code of the modified version.

An older license, called the Affero General Public License and published by Affero, was designed to accomplish
similar goals. This is a different license, not a version of the Affero GPL, but Affero has released a new version
of the Affero GPL which permits relicensing under this license.
```
