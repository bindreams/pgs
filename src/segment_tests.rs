use crate::segment::parse::read_segment;
use crate::segment::*;

fn make_segment_bytes(pts: u32, dts: u32, seg_type: u8, body: &[u8]) -> Vec<u8> {
    let mut buf = Vec::new();
    buf.extend_from_slice(b"PG");
    buf.extend_from_slice(&pts.to_be_bytes());
    buf.extend_from_slice(&dts.to_be_bytes());
    buf.push(seg_type);
    buf.extend_from_slice(&(body.len() as u16).to_be_bytes());
    buf.extend_from_slice(body);
    buf
}

#[skuld::test]
fn parse_end_segment() {
    let data = make_segment_bytes(1000, 900, 0x80, &[]);
    let seg = read_segment(&mut data.as_slice()).unwrap().unwrap();
    assert_eq!(seg.pts.as_ticks(), 1000);
    assert_eq!(seg.dts.as_ticks(), 900);
    assert!(seg.is_end());
}

#[skuld::test]
fn parse_pds() {
    // PDS: id=1, version=0, one entry: index=5, Y=16, Cr=128, Cb=128, Alpha=255
    let body = [1, 0, 5, 16, 128, 128, 255];
    let data = make_segment_bytes(0, 0, 0x14, &body);
    let seg = read_segment(&mut data.as_slice()).unwrap().unwrap();

    match &seg.body {
        SegmentBody::PaletteDefinition(pds) => {
            assert_eq!(pds.id, 1);
            assert_eq!(pds.version, 0);
            assert_eq!(pds.entries.len(), 1);
            assert_eq!(pds.entries[0].id, 5);
            assert_eq!(pds.entries[0].color.y, 16);
            assert_eq!(pds.entries[0].color.cr, 128);
            assert_eq!(pds.entries[0].color.cb, 128);
            assert_eq!(pds.entries[0].color.alpha, 255);
        }
        other => panic!("expected PaletteDefinition, got {other:?}"),
    }
}

#[skuld::test]
fn parse_wds() {
    // WDS: 1 window, id=0, x=100, y=200, w=300, h=50
    let mut body = vec![1u8]; // num_windows
    body.push(0); // id
    body.extend_from_slice(&100u16.to_be_bytes());
    body.extend_from_slice(&200u16.to_be_bytes());
    body.extend_from_slice(&300u16.to_be_bytes());
    body.extend_from_slice(&50u16.to_be_bytes());

    let data = make_segment_bytes(0, 0, 0x17, &body);
    let seg = read_segment(&mut data.as_slice()).unwrap().unwrap();

    match &seg.body {
        SegmentBody::WindowDefinition(wds) => {
            assert_eq!(wds.windows.len(), 1);
            assert_eq!(wds.windows[0].id, 0);
            assert_eq!(wds.windows[0].x, 100);
            assert_eq!(wds.windows[0].y, 200);
            assert_eq!(wds.windows[0].width, 300);
            assert_eq!(wds.windows[0].height, 50);
        }
        other => panic!("expected WindowDefinition, got {other:?}"),
    }
}

#[skuld::test]
fn parse_pcs_minimal() {
    // PCS: 1920x1080, framerate=0x10, comp_number=0, EpochStart, no palette update, 0 objects
    let mut body = Vec::new();
    body.extend_from_slice(&1920u16.to_be_bytes());
    body.extend_from_slice(&1080u16.to_be_bytes());
    body.push(0x10); // framerate
    body.extend_from_slice(&0u16.to_be_bytes()); // composition_number
    body.push(0x80); // EpochStart
    body.push(0x00); // palette_update_flag = false
    body.push(0x00); // palette_id (ignored when flag=0)
    body.push(0x00); // num_objects = 0

    let data = make_segment_bytes(90_000, 0, 0x16, &body);
    let seg = read_segment(&mut data.as_slice()).unwrap().unwrap();

    match &seg.body {
        SegmentBody::PresentationComposition(pcs) => {
            assert_eq!(pcs.width, 1920);
            assert_eq!(pcs.height, 1080);
            assert_eq!(pcs.state, CompositionState::EpochStart);
            assert!(pcs.palette_update_id.is_none());
            assert!(pcs.composition_objects.is_empty());
        }
        other => panic!("expected PCS, got {other:?}"),
    }
}

#[skuld::test]
fn parse_ods_single_fragment() {
    // ODS: id=0, version=0, First|Last, data_length=6 (4 bytes w/h + 2 bytes data)
    let mut body = Vec::new();
    body.extend_from_slice(&0u16.to_be_bytes()); // id
    body.push(0); // version
    body.push(0xC0); // First | Last
    // 3-byte data_length = 6
    body.push(0);
    body.push(0);
    body.push(6);
    // Data: width=2, height=1, then [0x05, 0x00, 0x00] (one pixel + EOL) -- but we just store raw
    body.extend_from_slice(&[0, 2, 0, 1, 0x05, 0x0A]);

    let data = make_segment_bytes(0, 0, 0x15, &body);
    let seg = read_segment(&mut data.as_slice()).unwrap().unwrap();

    match &seg.body {
        SegmentBody::ObjectDefinition(ods) => {
            assert_eq!(ods.id, 0);
            assert_eq!(ods.version, 0);
            assert!(ods.sequence.contains(SequenceFlag::FIRST));
            assert!(ods.sequence.contains(SequenceFlag::LAST));
            assert_eq!(ods.data, &[0, 2, 0, 1, 0x05, 0x0A]);
        }
        other => panic!("expected ODS, got {other:?}"),
    }
}

#[skuld::test]
fn parse_unknown_segment_type() {
    let data = make_segment_bytes(0, 0, 0xFF, &[]);
    let err = read_segment(&mut data.as_slice()).unwrap_err();
    assert!(matches!(
        err,
        crate::error::ReadError::UnknownSegmentType(0xFF)
    ));
}

#[skuld::test]
fn parse_eof_returns_none() {
    let result = read_segment(&mut [].as_slice()).unwrap();
    assert!(result.is_none());
}
