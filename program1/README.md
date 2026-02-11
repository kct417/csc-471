# Casey Tran

## Usage

```
mkdir build
cd build
```

```
cmake ..
make
```

```
./raster <meshfile> <imagefile> <width> <height> <mode>
```

## Known Issues
-   None

## Comments
-   The available modes are 0 and 1
    -   Mode 0 is default barycentric interpolation
    -   Mode 1 is triangle edge interpolation
-   It may be difficult to see the edges using edge interpolation with small images
    -   It is recommented that the images size be at least 1000 x 1000 pixels for mode 1
-   Mode one what coded using Bresenham's line algorithm with z-buffer depth incorporated into it
