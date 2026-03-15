use std::collections::HashMap;
use std::io::Read;

use crate::color::Rgba;
use crate::display_set::DisplaySet;
use crate::error::Error;
use crate::segment::parse::read_segment;
use crate::segment::*;
use crate::subtitle::Subtitle;

/// A reader that parses PGS segments from a byte stream.
///
/// Provides three levels of iteration:
/// - [`segments()`](SupReader::segments) — raw parsed segments
/// - [`display_sets()`](SupReader::display_sets) — aggregated display sets
/// - [`subtitles()`](SupReader::subtitles) — fully resolved RGBA subtitles
///
/// # Example
/// ```no_run
/// use std::fs::File;
/// use pgs::SupReader;
///
/// let reader = SupReader::new(File::open("subtitles.sup").unwrap());
/// for subtitle in reader.subtitles() {
///     let sub = subtitle.unwrap();
///     println!("{}x{} at ({},{})", sub.bitmap.width(), sub.bitmap.height(), sub.x, sub.y);
/// }
/// ```
pub struct SupReader<R> {
    reader: R,
}

impl<R: Read> SupReader<R> {
    pub fn new(reader: R) -> Self {
        Self { reader }
    }

    /// Returns an iterator over raw parsed segments.
    pub fn segments(self) -> Segments<R> {
        Segments {
            reader: self.reader,
            done: false,
        }
    }

    /// Returns an iterator over display sets (aggregated segments).
    pub fn display_sets(self) -> DisplaySets<R> {
        DisplaySets {
            segments: self.segments(),
            current: DisplaySet::empty(),
        }
    }

    /// Returns an iterator over fully-resolved subtitles.
    pub fn subtitles(self) -> Subtitles<R> {
        Subtitles {
            display_sets: self.display_sets(),
            pending: Vec::new(),
        }
    }
}

/// Iterator over individual PGS segments.
pub struct Segments<R> {
    reader: R,
    done: bool,
}

impl<R: Read> Iterator for Segments<R> {
    type Item = Result<Segment, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done {
            return None;
        }

        match read_segment(&mut self.reader) {
            Ok(Some(seg)) => Some(Ok(seg)),
            Ok(None) => {
                self.done = true;
                None
            }
            Err(e) => {
                self.done = true;
                Some(Err(Error::Read {
                    context: "failed to read segment".into(),
                    source: e,
                }))
            }
        }
    }
}

/// Iterator over display sets.
pub struct DisplaySets<R> {
    segments: Segments<R>,
    current: DisplaySet,
}

impl<R: Read> Iterator for DisplaySets<R> {
    type Item = Result<DisplaySet, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        let mut segment_buffer = Vec::new();

        for result in self.segments.by_ref() {
            let segment = match result {
                Ok(s) => s,
                Err(e) => return Some(Err(e)),
            };

            let is_end = segment.is_end();
            segment_buffer.push(segment);

            if is_end {
                let previous = self.current.clone();

                match self.current.update_from_segments(&segment_buffer) {
                    Ok(()) => {}
                    Err(e) => return Some(Err(e)),
                }

                // The previous display set's duration is the time until this one.
                let mut yielded = previous;
                if !yielded.is_empty() {
                    yielded.duration = self.current.timestamp - yielded.timestamp;
                    return Some(Ok(yielded));
                }

                // First display set (previous was empty) — continue to next
                segment_buffer.clear();
            }
        }

        // Yield the last display set
        if !self.current.is_empty() {
            let last = std::mem::replace(&mut self.current, DisplaySet::empty());
            Some(Ok(last))
        } else {
            None
        }
    }
}

/// Iterator over fully-resolved subtitles.
pub struct Subtitles<R> {
    display_sets: DisplaySets<R>,
    pending: Vec<Subtitle>,
}

impl<R: Read> Iterator for Subtitles<R> {
    type Item = Result<Subtitle, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if let Some(sub) = self.pending.pop() {
                return Some(Ok(sub));
            }

            let ds = match self.display_sets.next()? {
                Ok(ds) => ds,
                Err(e) => return Some(Err(e)),
            };

            match resolve_subtitles(&ds) {
                Ok(subs) => {
                    self.pending = subs;
                    self.pending.reverse(); // so we can pop from the end
                }
                Err(e) => return Some(Err(e)),
            }
        }
    }
}

/// Resolve a display set into subtitles by applying palette + RLE decoding.
fn resolve_subtitles(ds: &DisplaySet) -> Result<Vec<Subtitle>, Error> {
    let mut result = Vec::new();

    // Build a lookup table from the active palette
    let palette = ds.palettes.get(&ds.active_palette_id);
    let mut clut: HashMap<u8, Rgba> = HashMap::new();
    if let Some(pd) = palette {
        for entry in &pd.entries {
            clut.insert(entry.id, Rgba::from(entry.color));
        }
    }

    let get_color = |id: u8| -> Rgba {
        clut.get(&id).copied().unwrap_or(Rgba {
            r: 0,
            g: 0,
            b: 0,
            a: 0,
        })
    };

    for comp in &ds.compositions {
        let Some(object) = ds.objects.get(&comp.object_id) else {
            continue;
        };
        let Some(window) = ds.windows.get(&comp.window_id) else {
            continue;
        };

        let rgba_bitmap = object.map(|&idx| get_color(idx));

        // Apply cropping from composition object, then from window
        let mut bmp = rgba_bitmap;
        if let Some(crop) = &comp.crop {
            bmp = crop_bitmap(&bmp, crop.x, crop.y, crop.width, crop.height);
        }
        bmp = crop_bitmap(&bmp, 0, 0, window.width, window.height);

        result.push(Subtitle {
            bitmap: bmp,
            x: window.x,
            y: window.y,
            timestamp: ds.timestamp,
            duration: ds.duration,
        });
    }

    Ok(result)
}

fn crop_bitmap<T: Clone>(
    bmp: &crate::bitmap::Bitmap<T>,
    x: u16,
    y: u16,
    w: u16,
    h: u16,
) -> crate::bitmap::Bitmap<T> {
    let actual_w = w.min(bmp.width().saturating_sub(x));
    let actual_h = h.min(bmp.height().saturating_sub(y));

    if actual_w == 0 || actual_h == 0 {
        return crate::bitmap::Bitmap::new(0, 0, Vec::new());
    }

    let mut data = Vec::with_capacity(actual_w as usize * actual_h as usize);
    for row_y in y..y + actual_h {
        let row = bmp.row(row_y);
        data.extend_from_slice(&row[x as usize..(x + actual_w) as usize]);
    }

    crate::bitmap::Bitmap::new(actual_w, actual_h, data)
}

#[cfg(test)]
#[path = "read_tests.rs"]
mod tests;
