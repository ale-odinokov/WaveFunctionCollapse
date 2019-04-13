# WaveFunctionCollapse

C implementation of the [Wave Fucntion Collapse](https://github.com/mxgmn/WaveFunctionCollapse) bitmap generation algorithm.

Currently supported features and limitations:
* Only overlapping model
* Only periodic images for input and output
* Constraints can be defined with additional image file

Input/output image processing is based on PNG file format and uses [libpng](http://www.libpng.org/) library. The project can be compiled with GNU C compiler
```
gcc *.c -o wfc -lm -lpng
```

Some examples provided in the Samples directory:

|     Cats without any constraints     |
|  :---:  |
|  Input sample: |
| ![Cats](https://user-images.githubusercontent.com/23294701/56082795-d1e8b300-5e25-11e9-97d6-65e21456992e.png) |
|  Generating output:  |
|  ![cats_generation](https://user-images.githubusercontent.com/23294701/56082797-d4e3a380-5e25-11e9-8109-9ee733b8615f.gif)  |

|  Maze with predefined path  |
| :---: |
| Input sample + constraints as image: |
| ![Maze](https://user-images.githubusercontent.com/23294701/56082801-e0cf6580-5e25-11e9-98bb-7e945878cd17.png)  &nbsp;&nbsp;&nbsp;&nbsp;  ![maze_constraints](https://user-images.githubusercontent.com/23294701/56082802-e3ca5600-5e25-11e9-8de7-34d4c10a9b25.png)|
| Generating output: |
| ![maze_generation](https://user-images.githubusercontent.com/23294701/56082806-e62cb000-5e25-11e9-9417-98b29e3f0767.gif) |

| Flowers with predefined ground level |
| :---: |
| Input sample + constraints as image: |
| ![Flowers](https://user-images.githubusercontent.com/23294701/56082807-ec229100-5e25-11e9-84d7-e8f222b79a4f.png)  &nbsp;&nbsp;&nbsp;&nbsp;  ![flowers_constraints](https://user-images.githubusercontent.com/23294701/56082808-ef1d8180-5e25-11e9-903d-6c88bd1c6929.png) |
| Generating output: |
| ![flowers_generation](https://user-images.githubusercontent.com/23294701/56082809-f17fdb80-5e25-11e9-82a5-de92ca58f3c7.gif) |
