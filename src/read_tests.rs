use crate::SupReader;
use crate::segment::SegmentBody;
use std::fs::File;

fn sample_path(name: &str) -> std::path::PathBuf {
    let mut path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    path.push("tests");
    path.push("data");
    path.push(name);
    path
}

#[skuld::test]
fn read_segments_sample1() {
    let file = File::open(sample_path("sample-1.sup")).unwrap();
    let reader = SupReader::new(file);
    let mut count = 0;
    for seg in reader.segments() {
        seg.unwrap();
        count += 1;
    }
    assert!(count > 0, "expected at least one segment");
}

#[skuld::test]
fn read_segments_sample2() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let mut count = 0;
    for seg in reader.segments() {
        seg.unwrap();
        count += 1;
    }
    assert!(count > 0, "expected at least one segment");
}

#[skuld::test]
fn segments_contain_expected_types() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let mut has_pcs = false;
    let mut has_end = false;
    let mut has_pds = false;
    let mut has_ods = false;

    for seg in reader.segments() {
        let seg = seg.unwrap();
        match &seg.body {
            SegmentBody::PresentationComposition(_) => has_pcs = true,
            SegmentBody::End => has_end = true,
            SegmentBody::PaletteDefinition(_) => has_pds = true,
            SegmentBody::ObjectDefinition(_) => has_ods = true,
            _ => {}
        }
    }

    assert!(has_pcs, "expected PCS segments");
    assert!(has_end, "expected END segments");
    assert!(has_pds, "expected PDS segments");
    assert!(has_ods, "expected ODS segments");
}
