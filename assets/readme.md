[Compile this file 'ptf.c' to convert ASCII PLY files to C Header files.](ptf.c)

Compile:
`cc ptf.c -lm -Ofast -o ptf`
```
Usage: ./ptf filename_noextension
    (loads filenames from the 'ply/' directory)

Example: ./ptf porygon
    (loads 'ply/porygon.ply' and outputs 'porygon.h' into the cwd)
```
