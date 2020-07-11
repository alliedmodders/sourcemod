As of writing, we're using pcre v8.32

# Windows
We build with the MinSizeForRelease VS configuration using the target architecture you want. This is how both win libs are built. The following settings were selected in the CMAKE configuration
![AenKaUByp0](https://user-images.githubusercontent.com/11095737/87211804-10db1200-c2d0-11ea-85d9-ede4ba177de1.png)

In the PCRE project, go to C/C++ configuration properties and select the Runtime Library to be Multi-threaded (`/MT`) instead of Multi-threaded DLL (`/MD`)
