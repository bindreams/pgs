use super::*;

fn make_header(width: u16, height: u16) -> Vec<u8> {
    let mut v = Vec::new();
    v.extend_from_slice(&width.to_be_bytes());
    v.extend_from_slice(&height.to_be_bytes());
    v
}

#[skuld::test]
fn decode_single_colored_pixels() {
    // 3x1 bitmap: pixels [5, 10, 15]
    let mut data = make_header(3, 1);
    data.extend_from_slice(&[5, 10, 15, 0, 0]); // three pixels + EOL
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.width(), 3);
    assert_eq!(bmp.height(), 1);
    assert_eq!(bmp.data(), &[5, 10, 15]);
}

#[skuld::test]
fn decode_run_of_color_zero_short() {
    // 4x1 bitmap: [0, 0, 0, 0] encoded as short run
    let mut data = make_header(4, 1);
    // 00 04 = 4 pixels of color 0
    data.extend_from_slice(&[0x00, 0x04, 0x00, 0x00]); // run + EOL
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.data(), &[0, 0, 0, 0]);
}

#[skuld::test]
fn decode_run_of_color_zero_long() {
    // 100x1 bitmap: 100 pixels of color 0
    let mut data = make_header(100, 1);
    // 00 0b01_000000 0b01100100 = 100 pixels of color 0
    data.extend_from_slice(&[0x00, 0x40, 100, 0x00, 0x00]); // run + EOL
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.data().len(), 100);
    assert!(bmp.data().iter().all(|&v| v == 0));
}

#[skuld::test]
fn decode_run_of_nonzero_color_short() {
    // 5x1 bitmap: [42, 42, 42, 42, 42] encoded as short run
    let mut data = make_header(5, 1);
    // 00 0b10_000101 42 = 5 pixels of color 42
    data.extend_from_slice(&[0x00, 0x80 | 5, 42, 0x00, 0x00]); // run + EOL
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.data(), &[42, 42, 42, 42, 42]);
}

#[skuld::test]
fn decode_run_of_nonzero_color_long() {
    // 200x1: 200 pixels of color 7
    let mut data = make_header(200, 1);
    // 00 0b11_000000 0b11001000 07 = 200 pixels of color 7
    data.extend_from_slice(&[0x00, 0xC0, 200, 7, 0x00, 0x00]); // run + EOL
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.data().len(), 200);
    assert!(bmp.data().iter().all(|&v| v == 7));
}

#[skuld::test]
fn decode_multiple_rows() {
    // 2x2 bitmap
    let mut data = make_header(2, 2);
    // Row 0: pixels [1, 2]
    data.extend_from_slice(&[1, 2, 0, 0]);
    // Row 1: pixels [3, 4]
    data.extend_from_slice(&[3, 4, 0, 0]);
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.row(0), &[1, 2]);
    assert_eq!(bmp.row(1), &[3, 4]);
}

#[skuld::test]
fn decode_empty_buffer_error() {
    assert!(matches!(decode(&[]), Err(RleError::EmptyBuffer)));
    assert!(matches!(decode(&[0, 1]), Err(RleError::EmptyBuffer)));
}

#[skuld::test]
fn decode_row_count_mismatch() {
    // Header says 2 rows, but only 1 row of data
    let mut data = make_header(2, 2);
    data.extend_from_slice(&[1, 2, 0, 0]); // only 1 row
    let err = decode(&data).unwrap_err();
    assert!(matches!(
        err,
        RleError::RowCountMismatch {
            decoded: 1,
            expected: 2
        }
    ));
}

#[skuld::test]
fn decode_mixed_encoding() {
    // 6x1: [0, 0, 0, 5, 5, 5] using color-0 run + color-5 run
    let mut data = make_header(6, 1);
    // 00 03 = 3 pixels of color 0
    // 00 83 05 = 3 pixels of color 5
    data.extend_from_slice(&[0x00, 0x03, 0x00, 0x83, 0x05, 0x00, 0x00]);
    let bmp = decode(&data).unwrap();
    assert_eq!(bmp.data(), &[0, 0, 0, 5, 5, 5]);
}
