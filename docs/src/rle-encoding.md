# RLE Encoding

Object bitmaps in the [PGS format](pgs-format.md) are compressed using a run-length encoding (RLE) scheme[^rle-patent]. Each pixel in the decoded bitmap is an index (0–255) into a palette defined by a Palette Definition Segment.

## Data header

The compressed data begins with a 4-byte header:

| Size | Field                     |
| ---- | ------------------------- |
| 2    | Width (big-endian `u16`)  |
| 2    | Height (big-endian `u16`) |

The remaining bytes contain RLE-encoded scanlines.

## Encoding patterns

The encoding uses a zero byte as an escape character. A non-zero byte is a literal single-pixel value; a zero byte starts a multi-byte sequence:

| Bit pattern                           | Bytes | Meaning                         |
| ------------------------------------- | ----- | ------------------------------- |
| `CCCCCCCC` (C ≠ 0)                    | 1     | 1 pixel of color C              |
| `00000000 00LLLLLL` (L ≠ 0)           | 2     | L pixels of color 0 (L ≤ 63)    |
| `00000000 01LLLLLL LLLLLLLL`          | 3     | L pixels of color 0 (L ≤ 16383) |
| `00000000 10LLLLLL CCCCCCCC`          | 3     | L pixels of color C (L ≤ 63)    |
| `00000000 11LLLLLL LLLLLLLL CCCCCCCC` | 4     | L pixels of color C (L ≤ 16383) |
| `00000000 00000000`                   | 2     | End of line                     |

The two high bits of the second byte act as a **flag** that selects the variant:

| Flag (bits 7–6) | Run color            | Length encoding      |
| --------------- | -------------------- | -------------------- |
| `00`            | Color 0              | 6-bit length (short) |
| `01`            | Color 0              | 14-bit length (long) |
| `10`            | Color C (next byte)  | 6-bit length (short) |
| `11`            | Color C (final byte) | 14-bit length (long) |

## Decoding algorithm

1. Read a byte.
   - If it is **non-zero**, emit one pixel of that color.
   - If it is **zero**, read the next byte.
1. If the second byte is also **zero**, this is an **end-of-line** marker — advance to the next scanline.
1. Otherwise, extract the 2-bit flag from the high bits of the second byte:
   - The remaining 6 bits give the run length (short form) or the upper 6 bits of a 14-bit length (long form, combined with a third byte).
   - If the flag's MSB is set (`10` or `11`), a final byte specifies the color; otherwise the color is 0.
1. Emit the decoded run of pixels.
1. Repeat until all data is consumed.

## Line structure

Each scanline is terminated by the `00 00` end-of-line marker. After decoding, the number of scanlines must equal the **height** from the data header, and the total number of decoded pixels must equal **width × height**.

[^rle-patent]: [US Patent US7912305](https://patentimages.storage.googleapis.com/ab/c6/ed/195ad89b2b8f10/US7912305.pdf); [The Scorpius, "Presentation Graphic Stream (SUP files) — BluRay Subtitle Format"](https://blog.thescorpius.com/index.php/2017/07/15/presentation-graphic-stream-sup-files-bluray-subtitle-format/)
