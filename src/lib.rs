pub mod bitmap;
pub mod color;
pub mod display_set;
pub mod error;
pub mod read;
pub mod rle;
pub mod segment;
pub mod subtitle;
pub mod time;

pub use bitmap::{Bitmap, IndexedBitmap};
pub use color::{Rgba, YCbCrA};
pub use display_set::DisplaySet;
pub use error::Error;
pub use read::SupReader;
pub use segment::{Segment, SegmentBody};
pub use subtitle::Subtitle;
pub use time::{Duration, Timestamp, UNKNOWN_DURATION};

#[cfg(test)]
fn main() {
    skuld::run_all();
}
