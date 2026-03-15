pub mod parse;
pub mod types;

pub use types::*;

use crate::time::Timestamp;

/// A parsed PGS segment with presentation and decoding timestamps.
#[derive(Clone, Debug)]
pub struct Segment {
    pub pts: Timestamp,
    pub dts: Timestamp,
    pub body: SegmentBody,
}

/// The body of a PGS segment, distinguished by segment type.
#[derive(Clone, Debug)]
pub enum SegmentBody {
    PresentationComposition(PresentationComposition),
    WindowDefinition(WindowDefinition),
    PaletteDefinition(PaletteDefinition),
    ObjectDefinition(ObjectDefinition),
    End,
}

impl Segment {
    /// Returns `true` if this is an End-of-Display-Set segment.
    pub fn is_end(&self) -> bool {
        matches!(self.body, SegmentBody::End)
    }

    /// Returns the composition state if this is a PCS, otherwise `None`.
    pub fn composition_state(&self) -> Option<CompositionState> {
        match &self.body {
            SegmentBody::PresentationComposition(pcs) => Some(pcs.state),
            _ => None,
        }
    }
}
