use std::io;
use thiserror::Error;

#[derive(Debug, Error)]
#[non_exhaustive]
pub enum Error {
    #[error("I/O error: {0}")]
    Io(#[from] io::Error),

    #[error("{context}: {source}")]
    Read { context: String, source: ReadError },

    #[error("RLE decode error: {0}")]
    Rle(#[from] RleError),
}

#[derive(Debug, Error)]
pub enum ReadError {
    #[error("invalid magic bytes at offset {offset}: expected 'PG', got {got:#06x}")]
    InvalidMagic { offset: u64, got: u16 },

    #[error("unrecognized segment type {0:#04x}")]
    UnknownSegmentType(u8),

    #[error("unexpected end of input")]
    UnexpectedEof,

    #[error("invalid segment body: {0}")]
    InvalidBody(String),
}

#[derive(Debug, Error)]
pub enum RleError {
    #[error("empty RLE data buffer")]
    EmptyBuffer,

    #[error("row {row}: unexpected end of RLE data")]
    UnexpectedEnd { row: usize },

    #[error("decoded {decoded} rows but expected {expected}")]
    RowCountMismatch { decoded: usize, expected: u16 },

    #[error("decoded {decoded} pixels but expected {expected}")]
    PixelCountMismatch { decoded: usize, expected: usize },
}

pub type Result<T> = std::result::Result<T, Error>;

#[cfg(test)]
#[path = "error_tests.rs"]
mod tests;
