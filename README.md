# Image Viewer

A lightweight image viewer built from scratch in C using SDL3 for rendering. The project focuses on parsing and displaying image formats without relying on external image decoding libraries.

**Full PNG spec can be found here: https://www.w3.org/TR/png-3/

## Supported Formats
- PNG(In-Progress: 75%): I implement *most of the important elements in the full PNG spec; certain niche and obscure ancilliary chunks will not be implemented for obvious reasons.
  - Can view PLTE, RGBA, and Grayscale Images with full transparency and background color support. Interlacing, sub-8 bitdepth, dynamic color quantization still yet to be implemented.
- (maybe)JPG
- (maybe)BMP
## PNG Implementation

The PNG decoder handles the full pipeline from raw file to rendered pixels:

- Header validation and IHDR chunk parsing
- PLTE chunk support for indexed-color images
- IDAT chunk concatenation and zlib decompression
- Scanline filter byte handling
- Color type support for grayscale, RGB, indexed, and alpha variants

## Dependencies

- **SDL3** — windowing and rendering
- **zlib** — IDAT decompression (May implement from scratch in future, right now out of scope!)

## Building

```bash
git clone https://github.com/speakerchef/simple-image-viewer.git

cd simple-image-viewer
cmake -S . -B build

cd build
cmake --build .
```

## Usage

```bash
cd build
./image-viewer <path-to-image>
```

## License

MIT
