# Introduction

Kitserver 4 is a companion program for Pro Evolution Soccer 4 and Winning Eleven 8

# How to build

You need a modern Microsoft Visual C++ 32-bit compiler.
Visual Studio 2019 Community Edition is assumed in the setenv.vs2019.cmd example file, but you
should be able to use other versions of Visual Studio too.

Build is done from command-line.
Start cmd.exe, then switch to "src" directory, then:

    setenv.vs2019.cmd
    nmake
