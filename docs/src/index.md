:::\{toctree}
:hidden:
API Overview \<api-overview.md>
Python Bindings \<python.md>
PGS Format \<pgs-format.md>
RLE Encoding \<rle-encoding.md>
Color Conversion \<color-conversion.md>
License \<license.md>
:::

# pgs

Read PGS (Presentation Graphic Stream) subtitles from Blu-ray discs.

**PGS** is a bitmap-based subtitle format used in Blu-ray discs and stored in `.sup` files. Unlike text-based formats such as SRT, PGS subtitles are pre-rendered images positioned on a video frame, with colors stored in the Y'CbCr color space. This library parses the binary format, decodes RLE-compressed bitmaps, and converts palette colors to RGBA — producing ready-to-use subtitle images with position and timing information.

## Quick start

::::\{tab-set}
:::\{tab-item} Rust

```rust
use std::fs::File;
use pgs::SupReader;

let reader = SupReader::new(File::open("subtitles.sup").unwrap());
for subtitle in reader.subtitles() {
    let sub = subtitle.unwrap();
    println!("{}x{} at ({},{})", sub.bitmap.width(), sub.bitmap.height(), sub.x, sub.y);
}
```

:::
:::\{tab-item} Python

```python
import pgs

for sub in pgs.open("subtitles.sup").subtitles():
    print(f"{sub.bitmap.width}x{sub.bitmap.height} at ({sub.x},{sub.y})")
```

:::
::::

## Iteration levels

`SupReader` provides three levels of abstraction. Pick the one that matches your needs:

| Method            | Yields       | Description                                                                                            |
| ----------------- | ------------ | ------------------------------------------------------------------------------------------------------ |
| `.segments()`     | `Segment`    | Raw parsed segments — headers, palettes, bitmaps, and control data as they appear in the stream.       |
| `.display_sets()` | `DisplaySet` | Aggregated groups of segments that form a single display update, with accumulated state across epochs. |
| `.subtitles()`    | `Subtitle`   | Fully resolved RGBA bitmaps with screen position and timing, ready for rendering.                      |

## Internals

For details on the algorithms implemented by this library:

- [PGS Format](pgs-format.md) — segment header structure, segment types, and display set composition
- [RLE Encoding](rle-encoding.md) — run-length encoding scheme for subtitle bitmaps
- [Color Conversion](color-conversion.md) — BT.709 Y'CbCr to RGB conversion with math formulas

## License

This project is licensed under the Mozilla Public License 2.0 ([full text](license.md)).
