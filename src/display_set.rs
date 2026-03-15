use std::collections::HashMap;

use crate::bitmap::IndexedBitmap;
use crate::error::Error;
use crate::rle;
use crate::segment::*;
use crate::time::{Duration, Timestamp, UNKNOWN_DURATION};

/// A logical group of segments forming one display update.
///
/// Display sets carry state across an epoch: [`CompositionState::Normal`] and
/// [`CompositionState::AcquisitionPoint`] sets inherit windows, palettes, and
/// objects from previous sets. [`CompositionState::EpochStart`] resets all state.
#[derive(Clone, Debug)]
pub struct DisplaySet {
    pub timestamp: Timestamp,
    pub duration: Duration,
    pub state: CompositionState,
    pub windows: HashMap<u8, Window>,
    pub palettes: HashMap<u8, PaletteDefinition>,
    pub objects: HashMap<u16, IndexedBitmap>,
    pub compositions: Vec<CompositionObject>,
    pub active_palette_id: u8,
}

impl DisplaySet {
    /// Create an empty display set (used as initial state).
    pub(crate) fn empty() -> Self {
        Self {
            timestamp: Timestamp::from_ticks(0),
            duration: UNKNOWN_DURATION,
            state: CompositionState::EpochStart,
            windows: HashMap::new(),
            palettes: HashMap::new(),
            objects: HashMap::new(),
            compositions: Vec::new(),
            active_palette_id: 0,
        }
    }

    /// Returns `true` if this display set has no composition objects.
    pub fn is_empty(&self) -> bool {
        self.compositions.is_empty()
    }

    /// Update this display set from a buffer of segments (PCS..END).
    pub(crate) fn update_from_segments(&mut self, segments: &[Segment]) -> Result<(), Error> {
        debug_assert!(segments.len() >= 2);
        debug_assert!(matches!(
            segments[0].body,
            SegmentBody::PresentationComposition(_)
        ));
        debug_assert!(segments.last().unwrap().is_end());

        let pcs = match &segments[0].body {
            SegmentBody::PresentationComposition(pcs) => pcs,
            _ => unreachable!(),
        };

        // EpochStart resets all accumulated state
        if pcs.state == CompositionState::EpochStart {
            self.windows.clear();
            self.palettes.clear();
            self.objects.clear();
            self.active_palette_id = 0;
        }

        self.timestamp = segments[0].pts;
        self.duration = UNKNOWN_DURATION;
        self.state = pcs.state;
        self.compositions = pcs.composition_objects.clone();
        self.active_palette_id = pcs.palette_update_id.unwrap_or(self.active_palette_id);

        // Process intermediate segments (skip first PCS and last END)
        let mut incomplete_objects: HashMap<u16, Vec<u8>> = HashMap::new();

        for segment in &segments[1..segments.len() - 1] {
            match &segment.body {
                SegmentBody::WindowDefinition(wds) => {
                    for w in &wds.windows {
                        self.windows.insert(w.id, *w);
                    }
                }
                SegmentBody::PaletteDefinition(pds) => {
                    self.palettes.insert(pds.id, pds.clone());
                }
                SegmentBody::ObjectDefinition(ods) => {
                    if ods.sequence.contains(SequenceFlag::FIRST) {
                        incomplete_objects.insert(ods.id, ods.data.clone());
                    } else {
                        incomplete_objects
                            .entry(ods.id)
                            .or_default()
                            .extend_from_slice(&ods.data);
                    }

                    if ods.sequence.contains(SequenceFlag::LAST) {
                        if let Some(data) = incomplete_objects.remove(&ods.id) {
                            let bitmap = rle::decode(&data)?;
                            self.objects.insert(ods.id, bitmap);
                        }
                    }
                }
                _ => {}
            }
        }

        Ok(())
    }
}

#[cfg(test)]
#[path = "display_set_tests.rs"]
mod tests;
