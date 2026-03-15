# API Overview

This page walks through the public API of the `pgs` crate.

## SupReader

`SupReader` is the main entry point. It wraps any type that implements `std::io::Read` and provides three iterators at increasing levels of abstraction:

```rust
use std::fs::File;
use pgs::SupReader;

let reader = SupReader::new(File::open("subtitles.sup").unwrap());

// Pick one of:
let segments    = reader.segments();      // raw parsed segments
let display_sets = reader.display_sets(); // aggregated display sets
let subtitles   = reader.subtitles();     // fully resolved RGBA subtitles
```

All three are standard Rust iterators yielding `Result<T, pgs::Error>`, so they support streaming processing without loading the entire file into memory.

## Segments

The lowest level. Each `Segment` contains:

- `pts` / `dts` — presentation and decoding timestamps (`Timestamp`)
- `body` — a `SegmentBody` enum with five variants:

| Variant                        | Description                                                                                                                 |
| ------------------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| `PresentationComposition(PCS)` | Opens a display set: video dimensions, composition state, palette reference, and composition objects with screen positions. |
| `WindowDefinition(WDS)`        | Rectangular regions where objects can appear.                                                                               |
| `PaletteDefinition(PDS)`       | Up to 256 Y'CbCr+Alpha color entries.                                                                                       |
| `ObjectDefinition(ODS)`        | RLE-compressed bitmap data (may span multiple segments).                                                                    |
| `End`                          | Marks the end of a display set.                                                                                             |

See [PGS Format](pgs-format.md) for the binary layout of each segment type.

### CompositionState

The `PresentationComposition` segment carries a `CompositionState` that determines how the decoder manages buffered state:

- `EpochStart` — clears all accumulated windows, palettes, and objects.
- `AcquisitionPoint` — a full refresh that inherits existing state.
- `Normal` — an incremental update; only changed elements are included.

## Display sets

The middle level. `DisplaySets` aggregates segments into logical groups and maintains state across epochs:

```rust
for result in reader.display_sets() {
    let ds = result.unwrap();
    println!("{} objects, palette {}", ds.compositions.len(), ds.active_palette_id);
}
```

A `DisplaySet` contains:

| Field               | Type                             | Description                            |
| ------------------- | -------------------------------- | -------------------------------------- |
| `timestamp`         | `Timestamp`                      | When this display set appears.         |
| `duration`          | `Duration`                       | Time until the next display set.       |
| `state`             | `CompositionState`               | How this set relates to the epoch.     |
| `windows`           | `HashMap<u8, Window>`            | Accumulated window definitions.        |
| `palettes`          | `HashMap<u8, PaletteDefinition>` | Accumulated palette definitions.       |
| `objects`           | `HashMap<u16, IndexedBitmap>`    | Decoded palette-indexed bitmaps.       |
| `compositions`      | `Vec<CompositionObject>`         | What to draw and where.                |
| `active_palette_id` | `u8`                             | Which palette to use for color lookup. |

Multi-segment objects (ODS with `FIRST`/`LAST` flags) are automatically concatenated and RLE-decoded before being stored in the `objects` map.

## Subtitles

The highest level. Each `Subtitle` is a fully resolved image ready for rendering:

```rust
for result in reader.subtitles() {
    let sub = result.unwrap();
    // sub.bitmap: Bitmap<Rgba> — the rendered subtitle image
    // sub.x, sub.y             — screen position
    // sub.timestamp            — when to display
    // sub.duration             — how long to display
}
```

The resolution pipeline for each composition object:

1. Look up the decoded `IndexedBitmap` by object ID.
1. Map each palette index to `Rgba` using the active palette (see [Color Conversion](color-conversion.md)).
1. Apply cropping from the composition object, then from the window bounds.
1. Emit a `Subtitle` with the final RGBA bitmap, position, and timing.

## Supporting types

### Bitmap\<T>

A generic 2D pixel buffer. `Bitmap<u8>` (aliased as `IndexedBitmap`) holds palette indices; `Bitmap<Rgba>` holds resolved colors.

Key methods: `width()`, `height()`, `data()`, `row(y)`, `get(x, y)`, `map(f)`.

### YCbCrA and Rgba

Color types with bidirectional conversion via `From`:

```rust
let ycbcra = pgs::YCbCrA { y: 235, cr: 128, cb: 128, alpha: 255 };
let rgba = pgs::Rgba::from(ycbcra); // white
```

See [Color Conversion](color-conversion.md) for the math behind this.

### Timestamp and Duration

Time types based on the 90 kHz clock used in Blu-ray streams.

```rust
let ts = pgs::Timestamp::from_ticks(5400000); // 1 minute
println!("{}", ts); // "00:01:00.000"

let dur = pgs::Duration::from_millis(2500);
println!("{}", dur.as_ticks()); // 225000
```

`Timestamp` supports subtraction (yielding `Duration`) and addition with `Duration`.

## Error handling

All fallible operations return `pgs::Error`, which has three variants:

| Variant | Source           | When                                                                                          |
| ------- | ---------------- | --------------------------------------------------------------------------------------------- |
| `Io`    | `std::io::Error` | Reading from the underlying stream fails.                                                     |
| `Read`  | `ReadError`      | The binary data is malformed (bad magic, unknown segment type, unexpected EOF, invalid body). |
| `Rle`   | `RleError`       | RLE decoding fails (empty buffer, truncated data, row/pixel count mismatch).                  |
