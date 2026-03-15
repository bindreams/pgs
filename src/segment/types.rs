use crate::color::YCbCrA;
use bitflags::bitflags;

/// Presentation Composition Segment (PCS, type 0x16).
///
/// Describes what to display: screen dimensions, composition state, palette
/// reference, and a list of composition objects to render.
#[derive(Clone, Debug)]
pub struct PresentationComposition {
    pub width: u16,
    pub height: u16,
    pub framerate: u8,
    pub composition_number: u16,
    pub state: CompositionState,
    pub palette_update_id: Option<u8>,
    pub composition_objects: Vec<CompositionObject>,
}

/// The state of a presentation composition within an epoch.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CompositionState {
    /// Display update using only changed elements.
    Normal,
    /// Display refresh point — all elements are included for random access.
    AcquisitionPoint,
    /// Start of a new epoch — clears all buffered state.
    EpochStart,
}

impl CompositionState {
    pub(crate) fn from_byte(b: u8) -> Option<Self> {
        match b {
            0x00 => Some(Self::Normal),
            0x40 => Some(Self::AcquisitionPoint),
            0x80 => Some(Self::EpochStart),
            _ => None,
        }
    }
}

/// A reference to an object to be displayed at a position within a window.
#[derive(Clone, Debug)]
pub struct CompositionObject {
    pub object_id: u16,
    pub window_id: u8,
    pub x: u16,
    pub y: u16,
    pub crop: Option<CropRect>,
}

/// A cropping rectangle within an object.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CropRect {
    pub x: u16,
    pub y: u16,
    pub width: u16,
    pub height: u16,
}

/// Window Definition Segment (WDS, type 0x17).
///
/// Defines rectangular display regions on screen.
#[derive(Clone, Debug)]
pub struct WindowDefinition {
    pub windows: Vec<Window>,
}

/// A display window.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Window {
    pub id: u8,
    pub x: u16,
    pub y: u16,
    pub width: u16,
    pub height: u16,
}

/// Palette Definition Segment (PDS, type 0x14).
///
/// Defines a color lookup table mapping palette indices to YCbCrA colors.
#[derive(Clone, Debug)]
pub struct PaletteDefinition {
    pub id: u8,
    pub version: u8,
    pub entries: Vec<PaletteEntry>,
}

/// A single entry in a palette.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PaletteEntry {
    pub id: u8,
    pub color: YCbCrA,
}

/// Object Definition Segment (ODS, type 0x15).
///
/// Contains RLE-encoded bitmap data for a subtitle object. Large objects may be
/// split across multiple ODS segments (indicated by [`SequenceFlag`]).
#[derive(Clone, Debug)]
pub struct ObjectDefinition {
    pub id: u16,
    pub version: u8,
    pub sequence: SequenceFlag,
    /// Raw object data. For First segments, this includes the 4-byte width/height
    /// header followed by RLE-encoded pixels. Continuation segments contain only
    /// RLE data.
    pub data: Vec<u8>,
}

bitflags! {
    /// Flags indicating position of an ODS within a multi-segment object.
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct SequenceFlag: u8 {
        const LAST  = 0x40;
        const FIRST = 0x80;
    }
}
