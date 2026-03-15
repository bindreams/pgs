# Python Bindings

The `pgs` package provides Python bindings for this library via [PyO3](https://pyo3.rs). The Python API mirrors the Rust API, exposing all three iteration levels and supporting types.

## Installation

Build and install from source using [maturin](https://www.maturin.rs):

```bash
cd pgs-python
pip install .
```

Or for development:

```bash
cd pgs-python
maturin develop
```

## Quick start

```python
import pgs

for sub in pgs.open("subtitles.sup").subtitles():
    print(f"{sub.bitmap.width}x{sub.bitmap.height} at ({sub.x},{sub.y})")
```

## SupReader

`SupReader` is the main entry point. It accepts a file path (`str` or `os.PathLike`) or any file-like object with a `.read()` method:

```python
import pgs

# From a file path (fastest — uses Rust I/O directly)
reader = pgs.SupReader("subtitles.sup")

# From a pathlib.Path
from pathlib import Path
reader = pgs.SupReader(Path("subtitles.sup"))

# From a file-like object
with open("subtitles.sup", "rb") as f:
    reader = pgs.SupReader(f)

# From in-memory bytes
import io
reader = pgs.SupReader(io.BytesIO(data))
```

Each reader can be consumed exactly once by calling one of `.segments()`, `.display_sets()`, or `.subtitles()`. Calling a second method raises `ValueError`.

The module-level `pgs.open(source)` function is shorthand for `pgs.SupReader(source)`.

## Iteration levels

All three iterators follow the standard Python iterator protocol:

### Segments

```python
for seg in pgs.SupReader("subtitles.sup").segments():
    print(seg.body.kind)  # "presentation_composition", "window_definition", etc.

    if seg.body.kind == "presentation_composition":
        pcs = seg.body.as_presentation_composition()
        print(f"{pcs.width}x{pcs.height}, state={pcs.state}")
```

`SegmentBody` uses an accessor pattern. The `.kind` property returns a string, and typed methods like `.as_presentation_composition()` return `Optional[T]`:

| `.kind` value                | Accessor method                  | Returns                   |
| ---------------------------- | -------------------------------- | ------------------------- |
| `"presentation_composition"` | `.as_presentation_composition()` | `PresentationComposition` |
| `"window_definition"`        | `.as_window_definition()`        | `WindowDefinition`        |
| `"palette_definition"`       | `.as_palette_definition()`       | `PaletteDefinition`       |
| `"object_definition"`        | `.as_object_definition()`        | `ObjectDefinition`        |
| `"end"`                      | —                                | —                         |

### Display sets

```python
for ds in pgs.SupReader("subtitles.sup").display_sets():
    print(f"{ds.timestamp}: {len(ds.compositions)} objects, palette {ds.active_palette_id}")
    print(f"  windows: {ds.windows}")    # dict[int, Window]
    print(f"  palettes: {ds.palettes}")  # dict[int, PaletteDefinition]
```

### Subtitles

```python
for sub in pgs.SupReader("subtitles.sup").subtitles():
    # sub.bitmap   — Bitmap (RGBA pixel data)
    # sub.x, sub.y — screen position
    # sub.timestamp, sub.duration — timing
    pass
```

## Bitmap

`Bitmap` holds pixel data and exposes it in several ways:

```python
bmp = sub.bitmap
bmp.width    # int
bmp.height   # int
bmp.data     # bytes (RGBA, 4 bytes per pixel)
bmp.get(x, y)  # Rgba
bmp.row(y)     # bytes (one row of RGBA data)
len(bmp)       # pixel count (width * height)
```

### PIL/Pillow interop

```python
from PIL import Image

img = Image.frombytes("RGBA", (bmp.width, bmp.height), bmp.data)
img.save("subtitle.png")
```

### NumPy interop

`Bitmap` implements the buffer protocol, so `numpy` can wrap it with zero copy:

```python
import numpy as np

arr = np.frombuffer(bmp, dtype=np.uint8).reshape(bmp.height, bmp.width, 4)
```

## Time types

`Timestamp` and `Duration` are based on the 90 kHz PGS clock.

```python
ts = pgs.Timestamp(ticks=5400000)  # 1 minute
print(ts)          # "00:01:00.000"
print(ts.millis)   # 60000
print(ts.ticks)    # 5400000

dur = pgs.Duration.from_millis(2500)
print(dur.as_timedelta())  # datetime.timedelta(seconds=2, microseconds=500000)
```

Both types support comparison operators and hashing. `Timestamp` supports arithmetic with `Duration`:

```python
ts2 = ts + dur              # Timestamp
elapsed = ts2 - ts          # Duration
earlier = ts - dur          # Timestamp
```

The sentinel `pgs.UNKNOWN_DURATION` represents an unknown duration (used for the last subtitle in a stream).

## Color types

```python
rgba = pgs.Rgba(255, 255, 255, 255)
rgba.r, rgba.g, rgba.b, rgba.a   # individual components
rgba.to_tuple()                   # (255, 255, 255, 255)

ycbcra = rgba.to_ycbcra()         # BT.709 conversion
back = ycbcra.to_rgba()           # round-trip
```

## Error handling

Parsing errors raise Python exceptions:

| Exception      | When                                                          |
| -------------- | ------------------------------------------------------------- |
| `PgsReadError` | Malformed binary data (bad magic, unknown segment type, etc.) |
| `PgsRleError`  | RLE decoding fails (truncated data, row/pixel mismatch).      |
| `IOError`      | The underlying stream read fails.                             |

`PgsReadError` and `PgsRleError` both inherit from `PgsError`, which inherits from `RuntimeError`.

## Example: extract subtitles as PNG images

```python
import sys
from pathlib import Path

from PIL import Image

import pgs


def main():
    sup_path = sys.argv[1]
    out_dir = Path(Path(sup_path).stem)
    out_dir.mkdir(exist_ok=True)

    for i, sub in enumerate(pgs.open(sup_path).subtitles()):
        img = Image.frombytes("RGBA", (sub.bitmap.width, sub.bitmap.height), sub.bitmap.data)
        timestamp = str(sub.timestamp).replace(":", "-")
        img.save(out_dir / f"{i:04d}_{timestamp}.png")
        print(f"[{i:04d}] {sub.timestamp} ({sub.bitmap.width}x{sub.bitmap.height})")

    print(f"Done — saved to {out_dir}/")


if __name__ == "__main__":
    main()
```

Usage:

```bash
python extract_subs.py subtitles.sup
```

This iterates over all subtitles, converts each RGBA bitmap to a PIL `Image`, and saves it as a PNG. Output files are named like `0000_00-01-23.456.png` in a folder named after the input file.
