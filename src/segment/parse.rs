use std::io::{self, Read};

use super::types::*;
use super::{Segment, SegmentBody};
use crate::color::YCbCrA;
use crate::error::ReadError;
use crate::time::Timestamp;

/// Parse a single segment from a reader.
///
/// Returns `Ok(None)` on clean EOF (no bytes available), `Ok(Some(segment))` on
/// success, or `Err` on parse failure.
pub fn read_segment<R: Read>(reader: &mut R) -> Result<Option<Segment>, ReadError> {
    // Read the 13-byte header: "PG" (2) + PTS (4) + DTS (4) + type (1) + size (2)
    let mut header_buf = [0u8; 13];
    match reader.read_exact(&mut header_buf) {
        Ok(()) => {}
        Err(e) if e.kind() == io::ErrorKind::UnexpectedEof => return Ok(None),
        Err(_) => return Err(ReadError::UnexpectedEof),
    }

    // Validate magic
    if header_buf[0] != b'P' || header_buf[1] != b'G' {
        let got = u16::from_be_bytes([header_buf[0], header_buf[1]]);
        return Err(ReadError::InvalidMagic { offset: 0, got });
    }

    let pts = u32::from_be_bytes([header_buf[2], header_buf[3], header_buf[4], header_buf[5]]);
    let dts = u32::from_be_bytes([header_buf[6], header_buf[7], header_buf[8], header_buf[9]]);
    let segment_type = header_buf[10];
    let segment_size = u16::from_be_bytes([header_buf[11], header_buf[12]]);

    // Read the body bytes
    let mut body = vec![0u8; segment_size as usize];
    reader
        .read_exact(&mut body)
        .map_err(|_| ReadError::UnexpectedEof)?;

    let pts = Timestamp::from_ticks(pts);
    let dts = Timestamp::from_ticks(dts);

    let segment_body = match segment_type {
        0x14 => parse_pds(&body)?,
        0x15 => parse_ods(&body)?,
        0x16 => parse_pcs(&body)?,
        0x17 => parse_wds(&body)?,
        0x80 => SegmentBody::End,
        other => return Err(ReadError::UnknownSegmentType(other)),
    };

    Ok(Some(Segment {
        pts,
        dts,
        body: segment_body,
    }))
}

// PCS parsing =========================================================================================================

fn parse_pcs(body: &[u8]) -> Result<SegmentBody, ReadError> {
    if body.len() < 11 {
        return Err(ReadError::InvalidBody("PCS body too short".into()));
    }

    let width = u16::from_be_bytes([body[0], body[1]]);
    let height = u16::from_be_bytes([body[2], body[3]]);
    let framerate = body[4];
    let composition_number = u16::from_be_bytes([body[5], body[6]]);
    let state = CompositionState::from_byte(body[7]).ok_or_else(|| {
        ReadError::InvalidBody(format!("unknown composition state {:#04x}", body[7]))
    })?;
    let palette_update_flag = body[8];
    let palette_id = body[9];
    let num_objects = body[10] as usize;

    let palette_update_id = if palette_update_flag != 0 {
        Some(palette_id)
    } else {
        None
    };

    let mut pos = 11;
    let mut composition_objects = Vec::with_capacity(num_objects);

    for _ in 0..num_objects {
        if pos + 8 > body.len() {
            return Err(ReadError::InvalidBody(
                "PCS: composition object truncated".into(),
            ));
        }

        let object_id = u16::from_be_bytes([body[pos], body[pos + 1]]);
        let window_id = body[pos + 2];
        let cropped_flag = body[pos + 3];
        let x = u16::from_be_bytes([body[pos + 4], body[pos + 5]]);
        let y = u16::from_be_bytes([body[pos + 6], body[pos + 7]]);
        pos += 8;

        let crop = if cropped_flag & 0x40 != 0 {
            if pos + 8 <= body.len() {
                let crop = CropRect {
                    x: u16::from_be_bytes([body[pos], body[pos + 1]]),
                    y: u16::from_be_bytes([body[pos + 2], body[pos + 3]]),
                    width: u16::from_be_bytes([body[pos + 4], body[pos + 5]]),
                    height: u16::from_be_bytes([body[pos + 6], body[pos + 7]]),
                };
                pos += 8;
                Some(crop)
            } else {
                // Some subtitle tracks set the crop flag but omit the crop data.
                // The C++ implementation warns and continues.
                eprintln!("Warning: composition object has cropped flag set but no crop data");
                None
            }
        } else {
            None
        };

        composition_objects.push(CompositionObject {
            object_id,
            window_id,
            x,
            y,
            crop,
        });
    }

    Ok(SegmentBody::PresentationComposition(
        PresentationComposition {
            width,
            height,
            framerate,
            composition_number,
            state,
            palette_update_id,
            composition_objects,
        },
    ))
}

// WDS parsing =========================================================================================================

fn parse_wds(body: &[u8]) -> Result<SegmentBody, ReadError> {
    if body.is_empty() {
        return Err(ReadError::InvalidBody("WDS body empty".into()));
    }

    let num_windows = body[0] as usize;
    let mut pos = 1;
    let mut windows = Vec::with_capacity(num_windows);

    for _ in 0..num_windows {
        if pos + 9 > body.len() {
            return Err(ReadError::InvalidBody("WDS: window entry truncated".into()));
        }

        windows.push(Window {
            id: body[pos],
            x: u16::from_be_bytes([body[pos + 1], body[pos + 2]]),
            y: u16::from_be_bytes([body[pos + 3], body[pos + 4]]),
            width: u16::from_be_bytes([body[pos + 5], body[pos + 6]]),
            height: u16::from_be_bytes([body[pos + 7], body[pos + 8]]),
        });
        pos += 9;
    }

    Ok(SegmentBody::WindowDefinition(WindowDefinition { windows }))
}

// PDS parsing =========================================================================================================

fn parse_pds(body: &[u8]) -> Result<SegmentBody, ReadError> {
    if body.len() < 2 {
        return Err(ReadError::InvalidBody("PDS body too short".into()));
    }

    let id = body[0];
    let version = body[1];
    let mut pos = 2;
    let mut entries = Vec::new();

    // Each palette entry is 5 bytes: id, Y, Cr, Cb, Alpha
    while pos + 5 <= body.len() {
        entries.push(PaletteEntry {
            id: body[pos],
            color: YCbCrA {
                y: body[pos + 1],
                cr: body[pos + 2],
                cb: body[pos + 3],
                alpha: body[pos + 4],
            },
        });
        pos += 5;
    }

    Ok(SegmentBody::PaletteDefinition(PaletteDefinition {
        id,
        version,
        entries,
    }))
}

// ODS parsing =========================================================================================================

fn parse_ods(body: &[u8]) -> Result<SegmentBody, ReadError> {
    if body.len() < 4 {
        return Err(ReadError::InvalidBody("ODS body too short".into()));
    }

    let id = u16::from_be_bytes([body[0], body[1]]);
    let version = body[2];
    let sequence = SequenceFlag::from_bits_truncate(body[3]);

    let data = if sequence.contains(SequenceFlag::FIRST) {
        // First segment has a 3-byte data_length field, then width/height + RLE data
        if body.len() < 7 {
            return Err(ReadError::InvalidBody(
                "ODS: first segment too short for data_length".into(),
            ));
        }
        let data_length =
            ((body[4] as usize) << 16) | ((body[5] as usize) << 8) | (body[6] as usize);
        let remaining = body.len() - 7;
        if remaining != data_length {
            return Err(ReadError::InvalidBody(format!(
                "ODS: data_length ({data_length}) does not match remaining body size ({remaining})"
            )));
        }
        body[7..].to_vec()
    } else {
        // Continuation segment: everything after the 4-byte header is data
        body[4..].to_vec()
    };

    Ok(SegmentBody::ObjectDefinition(ObjectDefinition {
        id,
        version,
        sequence,
        data,
    }))
}

#[cfg(test)]
#[path = "../segment_tests.rs"]
mod tests;
