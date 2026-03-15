# PGS Binary Format

PGS (Presentation Graphic Stream) is a bitmap-based subtitle format used in Blu-ray discs[^pgs-patent]. Subtitle data is stored as a sequence of **segments**, each carrying a piece of the display update: composition metadata, window definitions, palettes, or compressed bitmap data. Files containing PGS data typically use the `.sup` extension.

## Segment header

Every segment begins with a 13-byte header:

| Offset | Size | Field        | Description                                                          |
| ------ | ---- | ------------ | -------------------------------------------------------------------- |
| 0      | 2    | Magic number | Always `0x50 0x47` (ASCII "PG")                                      |
| 2      | 4    | PTS          | Presentation timestamp. 90 kHz clock; divide by 90 for milliseconds. |
| 6      | 4    | DTS          | Decoding timestamp. Typically `0x00000000`.                          |
| 10     | 1    | Segment type | Identifies the segment body format (see below).                      |
| 11     | 2    | Segment size | Length of the body following this header, in bytes.                  |

All multi-byte integers are big-endian.

## Segment types

| Type | Hex    | Name                             |
| ---- | ------ | -------------------------------- |
| PCS  | `0x16` | Presentation Composition Segment |
| WDS  | `0x17` | Window Definition Segment        |
| PDS  | `0x14` | Palette Definition Segment       |
| ODS  | `0x15` | Object Definition Segment        |
| END  | `0x80` | End of Display Set               |

### Presentation Composition Segment (PCS)

The PCS opens every display set and describes what should appear on screen.

| Size | Field                         | Description                                                 |
| ---- | ----------------------------- | ----------------------------------------------------------- |
| 2    | Video width                   | Frame width in pixels (e.g. 1920)                           |
| 2    | Video height                  | Frame height in pixels (e.g. 1080)                          |
| 1    | Frame rate                    | Always `0x10`; not used in practice                         |
| 2    | Composition number            | Incremented on each graphics update                         |
| 1    | Composition state             | `0x00` Normal, `0x40` Acquisition Point, `0x80` Epoch Start |
| 1    | Palette update flag           | `0x80` if this is a palette-only update, `0x00` otherwise   |
| 1    | Palette ID                    | References a palette defined in a PDS                       |
| 1    | Number of composition objects | Count of objects in this segment                            |

**Composition state** controls how the decoder handles buffered state:

- **Epoch Start** (`0x80`): Clears all previously buffered windows, palettes, and objects. All segments needed for display must be present in this display set. This is the only safe random-access entry point.
- **Acquisition Point** (`0x40`): A mid-epoch refresh. Contains all objects needed for the current display, but does not reset buffered state.
- **Normal** (`0x00`): An incremental update. Only changed elements are included; everything else is inherited from the previous display set.

Each **composition object** is 8 bytes (or 16 if cropped):

| Size | Field                                          |
| ---- | ---------------------------------------------- |
| 2    | Object ID (references an ODS)                  |
| 1    | Window ID (references a window in the WDS)     |
| 1    | Object cropped flag (`0x40` = cropped)         |
| 2    | Horizontal position (X from top-left of frame) |
| 2    | Vertical position (Y from top-left of frame)   |

If the cropped flag is `0x40`, four additional fields follow:

| Size | Field                    |
| ---- | ------------------------ |
| 2    | Crop horizontal position |
| 2    | Crop vertical position   |
| 2    | Crop width               |
| 2    | Crop height              |

### Window Definition Segment (WDS)

Defines rectangular regions on the video frame where subtitle objects can appear.

| Size | Field             |
| ---- | ----------------- |
| 1    | Number of windows |

Each window (9 bytes):

| Size | Field               |
| ---- | ------------------- |
| 1    | Window ID           |
| 2    | Horizontal position |
| 2    | Vertical position   |
| 2    | Width               |
| 2    | Height              |

### Palette Definition Segment (PDS)

Defines a palette of up to 256 colors in the Y'CbCr color space.

| Size | Field                  |
| ---- | ---------------------- |
| 1    | Palette ID             |
| 1    | Palette version number |

Each palette entry (5 bytes):

| Size | Field                                 |
| ---- | ------------------------------------- |
| 1    | Entry ID (index 0–255)                |
| 1    | Y (luminance)                         |
| 1    | Cr (red-difference chroma)            |
| 1    | Cb (blue-difference chroma)           |
| 1    | Alpha (0 = transparent, 255 = opaque) |

:::\{note}
The wire order is Y, **Cr**, **Cb** — not Y, Cb, Cr. See [Color Conversion](color-conversion.md) for how these values are converted to RGB.
:::

### Object Definition Segment (ODS)

Contains a bitmap compressed with [run-length encoding](rle-encoding.md).

| Size | Field                                                     |
| ---- | --------------------------------------------------------- |
| 2    | Object ID                                                 |
| 1    | Object version number                                     |
| 1    | Sequence flag                                             |
| 3    | Object data length (total, including width/height header) |

The **sequence flag** is a bitfield:

| Value  | Meaning                                          |
| ------ | ------------------------------------------------ |
| `0x80` | First segment of this object                     |
| `0x40` | Last segment of this object                      |
| `0xC0` | First and last (object fits in a single segment) |

When the `FIRST` flag is set, the data begins with a 4-byte header:

| Size | Field         |
| ---- | ------------- |
| 2    | Bitmap width  |
| 2    | Bitmap height |

The remaining bytes are RLE-compressed bitmap data. Objects too large for a single segment are split across multiple ODS segments sharing the same object ID; the `FIRST`/`LAST` flags indicate where the data begins and ends.

### End of Display Set (END)

An empty segment (zero-length body) that marks the end of a display set. No fields beyond the segment header.

## Display sets

Segments are grouped into **display sets**. A display set always begins with a PCS and ends with an END segment. The typical order is:

```
PCS → WDS → PDS → ODS [→ ODS ...] → END
```

The decoder maintains a buffer of windows, palettes, and decoded objects. How the buffer is updated depends on the PCS composition state:

```
Epoch Start:        PCS(0x80) → WDS → PDS → ODS → END     ← clears all state
Acquisition Point:  PCS(0x40) → WDS → PDS → ODS → END     ← inherits state
Normal:             PCS(0x00) → PDS → END                  ← palette update only
Normal:             PCS(0x00) → END                        ← clears display
```

Each display set's **duration** is the time between its PTS and the next display set's PTS.

[^pgs-patent]: [US Patent US20090185789A1](https://patentimages.storage.googleapis.com/3e/b5/03/01caa801c398a8/US20090185789A1.pdf); [The Scorpius, "Presentation Graphic Stream (SUP files) — BluRay Subtitle Format"](https://blog.thescorpius.com/index.php/2017/07/15/presentation-graphic-stream-sup-files-bluray-subtitle-format/)
