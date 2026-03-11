# Image Viewer

A lightweight image viewer built from scratch in C using SDL3 for rendering. The project focuses on parsing, pre-processing raw image data, and displaying image formats without relying on external image decoding libraries.

**Full PNG spec can be found here: https://www.w3.org/TR/png-3/

## Supported Formats
- **PNG**: Full support for the most common and important elements of the PNG spec. Certain niche ancillary chunks (iTXt, sPLT, etc.) are not implemented.
  - All standard color types: Grayscale, RGB, Palette, Grayscale+Alpha, RGBA
  - 8-bit and 16-bit channel depth
  - All 5 scanline filter types (None, Sub, Up, Average, Paeth)
  - Full transparency (tRNS) and background color (bKGD) support
  - HDR via cICP — BT.2100 PQ and HLG transfer functions with BT.2020 to BT.709 gamut mapping
  - sRGB and iCCP chunk detection with color space priority handling
  - Automatic display scaling with aspect ratio preservation
  - **Not supported:** Adam7 interlacing, sub-8 bit depth, gAMA chunk application
- (maybe) JPG
- (maybe) BMP

## PNG Implementation

The PNG decoder handles the full pipeline from raw file to rendered pixels:
- Header validation and IHDR chunk parsing
- PLTE chunk support for indexed-color images
- IDAT chunk concatenation and zlib decompression
- All 5 scanline filter types: None, Sub, Up, Average, Paeth
- Color type support for grayscale, RGB, indexed, and alpha variants at 8 and 16 bits per channel
- Ancillary chunk handling: bKGD, tRNS, cICP, sRGB, iCCP(partial), gAMA(only detection)
- Color space priority: iCCP > sRGB > cICP > gAMA (per PNG spec)

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
