# `dualis-bmp`

This example shows how to use `dualis` to read the meta data of a Windows bitmap file (`.bmp`) and display it.

## Usage

To show the meta information of a bitmap file, use:

`dualis-bmp <file.bmp>`

where `<file.bmp>` is the path to said bitmap file. The following shows an example of its output:

```
This bitmap is 1920x2684 @ 4bpp.
It is not compressed, taking up 2576640 bytes.
It is indexed with the following palette:
  [  0] #493d5a [  1] #130b2e [  2] #1f1757 [  3] #512264 [  4] #a93e61 [  5] #629271 [  6] #f6e942 [  7] #3950bd
  [  8] #282891 [  9] #5a289a [ 10] #5d579f [ 11] #af52ba [ 12] #38b3dc [ 13] #caa9ca [ 14] #a2a7e3 [ 15] #ebe5f4
```

## Notes

This example reads the so-called `BitmapInfoHeader` once using individual reads for each field, and once using a raw read that reads in the entire `struct` at once.
It then compares the two.
On most common machine architectures (namely x64 and ARM), which are little endian, the two should be equal.
However, this is not the case on big-endian architectures.
In addition, the size of the `BitmapInfoHeader` is statically checked using `static_assert` to ensure the compiler does not screw up the alignment of the individual fields.
In conclusion, the version reading the individual fields is portable, whereas the other one is not.