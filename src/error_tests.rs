use super::*;

#[skuld::test]
fn rle_error_display() {
    let e = RleError::EmptyBuffer;
    assert_eq!(e.to_string(), "empty RLE data buffer");
}

#[skuld::test]
fn rle_error_unexpected_end() {
    let e = RleError::UnexpectedEnd { row: 5 };
    assert_eq!(e.to_string(), "row 5: unexpected end of RLE data");
}

#[skuld::test]
fn rle_error_row_count_mismatch() {
    let e = RleError::RowCountMismatch {
        decoded: 3,
        expected: 5,
    };
    assert_eq!(e.to_string(), "decoded 3 rows but expected 5");
}

#[skuld::test]
fn read_error_unknown_segment_type() {
    let e = ReadError::UnknownSegmentType(0xFF);
    assert_eq!(e.to_string(), "unrecognized segment type 0xff");
}

#[skuld::test]
fn error_from_io() {
    let io_err = std::io::Error::new(std::io::ErrorKind::NotFound, "file not found");
    let e: Error = io_err.into();
    assert!(matches!(e, Error::Io(_)));
}

#[skuld::test]
fn error_from_rle() {
    let rle_err = RleError::EmptyBuffer;
    let e: Error = rle_err.into();
    assert!(matches!(e, Error::Rle(_)));
}
