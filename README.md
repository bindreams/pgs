# pgs

Read PGS (Presentation Graphic Stream) subtitles from Blu-ray discs.

**PGS** is a bitmap-based subtitle format embedded in Blu-ray disc streams, typically stored in `.sup` files. Unlike text-based formats like SRT, PGS subtitles are pre-rendered images. This library parses the binary format, decodes RLE-compressed bitmaps, and converts Y'CbCr palette colors to RGBA — producing ready-to-use subtitle images with position and timing.

## Quick start

```rust
use std::fs::File;
use pgs::SupReader;

let reader = SupReader::new(File::open("subtitles.sup").unwrap());
for subtitle in reader.subtitles() {
    let sub = subtitle.unwrap();
    println!("{}x{} at ({},{})", sub.bitmap.width(), sub.bitmap.height(), sub.x, sub.y);
}
```

## Iteration levels

`SupReader` provides three levels of abstraction:

| Method            | Yields       | Description                                                                 |
| ----------------- | ------------ | --------------------------------------------------------------------------- |
| `.segments()`     | `Segment`    | Raw parsed segments as they appear in the stream.                           |
| `.display_sets()` | `DisplaySet` | Groups of segments forming a single display update, with accumulated state. |
| `.subtitles()`    | `Subtitle`   | Fully resolved RGBA bitmaps with screen position and timing.                |

## License

MPL-2.0
