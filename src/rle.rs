use crate::bitmap::IndexedBitmap;
use crate::error::RleError;

/// Decode RLE-compressed data into a palette-indexed bitmap.
///
/// The input `data` should start with a 4-byte header (width u16 BE, height u16 BE)
/// followed by RLE-encoded scanlines.
pub fn decode(data: &[u8]) -> Result<IndexedBitmap, RleError> {
    if data.len() < 4 {
        return Err(RleError::EmptyBuffer);
    }

    let width = u16::from_be_bytes([data[0], data[1]]) as usize;
    let height = u16::from_be_bytes([data[2], data[3]]) as usize;
    let total_size = width * height;

    let mut result = Vec::with_capacity(total_size);
    let rle_data = &data[4..];
    let mut pos = 0;
    let mut row_i = 0;

    let advance = |pos: &mut usize, row_i: usize| -> Result<u8, RleError> {
        if *pos >= rle_data.len() {
            return Err(RleError::UnexpectedEnd { row: row_i });
        }
        let b = rle_data[*pos];
        *pos += 1;
        Ok(b)
    };

    while pos < rle_data.len() {
        // Decode a single line ----------------------------------------------------------------------------------------
        loop {
            let byte0 = advance(&mut pos, row_i)?;

            if byte0 != 0 {
                // CCCCCCCC: one pixel in color C
                result.push(byte0);
                continue;
            }

            let byte1 = advance(&mut pos, row_i)?;

            if byte1 == 0 {
                // 00000000 00000000: end of line
                break;
            }

            let flag = byte1 >> 6;
            let byte1_clean = (byte1 & 0b0011_1111) as usize;
            let n_pixels: usize;
            let color: u8;

            match flag {
                0b00 => {
                    // 00000000 00LLLLLL: L pixels in color 0
                    n_pixels = byte1_clean;
                    color = 0;
                }
                0b01 => {
                    // 00000000 01LLLLLL LLLLLLLL: L pixels in color 0
                    let byte2 = advance(&mut pos, row_i)?;
                    n_pixels = (byte1_clean << 8) | byte2 as usize;
                    color = 0;
                }
                0b10 => {
                    // 00000000 10LLLLLL CCCCCCCC: L pixels in color C
                    n_pixels = byte1_clean;
                    let byte2 = advance(&mut pos, row_i)?;
                    color = byte2;
                }
                0b11 => {
                    // 00000000 11LLLLLL LLLLLLLL CCCCCCCC: L pixels in color C
                    let byte2 = advance(&mut pos, row_i)?;
                    n_pixels = (byte1_clean << 8) | byte2 as usize;
                    let byte3 = advance(&mut pos, row_i)?;
                    color = byte3;
                }
                _ => unreachable!(),
            }

            result.extend(std::iter::repeat_n(color, n_pixels));
        }

        row_i += 1;
    }

    if row_i != height {
        return Err(RleError::RowCountMismatch {
            decoded: row_i,
            expected: height as u16,
        });
    }

    if result.len() != total_size {
        return Err(RleError::PixelCountMismatch {
            decoded: result.len(),
            expected: total_size,
        });
    }

    Ok(IndexedBitmap::new(width as u16, height as u16, result))
}

#[cfg(test)]
#[path = "rle_tests.rs"]
mod tests;
