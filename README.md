# Image Viewer

A lightweight image viewer built from scratch in C using SDL3 for rendering. The project focuses on parsing, pre-processing raw image data, and displaying image formats without relying on external image decoding libraries.

**Full PNG spec can be found here: https://www.w3.org/TR/png-3/

## Supported Formats
- PNG (In-Progress: ~75%): I implement most of the important elements in the full PNG spec; certain niche and obscure ancillary chunks will not be implemented for obvious reasons.
  - Can view PLTE, RGB, RGBA, Grayscale, and Grayscale+Alpha images with full transparency and background color support
  - 8-bit and 16-bit channel depth support
  - All 5 scanline filter types (None, Sub, Up, Average, Paeth)
  - HDR support via cICP chunks — BT.2100 Perceptual Quantization (PQ) and Hybrid Log-Gamma (HLG) transfer functions with BT.2020 to BT.709 gamut mapping
  - Interlacing, sub-8 bit depth, and dynamic color quantization still yet to be implemented
- (maybe) JPG
- (maybe) BMP

## PNG Implementation

The PNG decoder handles the full pipeline from raw file to rendered pixels:
- Header validation and IHDR chunk parsing
- PLTE chunk support for indexed-color images
- IDAT chunk concatenation and zlib decompression
- All 5 scanline filter types: None, Sub, Up, Average, Paeth
- Color type support for grayscale, RGB, indexed, and alpha variants at 8 and 16 bits per channel
- Ancillary chunk handling: bKGD (background color), tRNS (transparency), cICP (HDR color information)

## HDR Support

For images with cICP chunks signaling BT.2100 color:
- **PQ (Perceptual Quantization):** SMPTE ST 2084 EOTF decoding from 10,000-nit absolute luminance to display-referred linear light, normalized against a reference white point
- **HLG (Hybrid Log-Gamma):** BT.2100 inverse OETF with full OOTF applying luminance-weighted system gamma for scene-to-display conversion
- BT.2020 to BT.709 gamut conversion via a hand-derived color matrix using CIE chromaticity coordinates and D65 white point

## Dependencies

- **SDL3** — windowing and rendering
- **zlib** — IDAT decompression (may implement from scratch in future, right now out of scope!)

## Building

```bash
git clone https://github.com/speakerchef/simple-image-viewer.git
cd simple-image-viewer
cmake -S . -B build
cd build && cmake --build .
```

## Usage

```bash
./build/image-viewer <path-to-image>
```

## License

MIT
