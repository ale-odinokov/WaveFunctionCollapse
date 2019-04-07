# WaveFunctionCollapse

C implementation of the [Wave Fucntion Collapse](https://github.com/mxgmn/WaveFunctionCollapse) bitmap generation algorithm.

Currently supported features and limitations:
* Only overlapping model
* Only periodic images for input and output
* Constraints can be defined with additional image file

Input/output image processing is based on PNG file format using [libpng](http://www.libpng.org/) library. The project can be compiled with GNU C compiler
```
gcc *.c -o wfc -lm -lpng
```

