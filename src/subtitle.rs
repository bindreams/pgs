use crate::bitmap::Bitmap;
use crate::color::Rgba;
use crate::time::{Duration, Timestamp};

/// A fully resolved subtitle: an RGBA bitmap with position and timing.
#[derive(Clone, Debug)]
pub struct Subtitle {
    pub bitmap: Bitmap<Rgba>,
    pub x: u16,
    pub y: u16,
    pub timestamp: Timestamp,
    pub duration: Duration,
}
