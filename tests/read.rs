use std::fs::File;
use std::path::PathBuf;

use pgs::SupReader;
use pgs::segment::SegmentBody;
use pgs::time::UNKNOWN_DURATION;

fn main() {
    skuld::run_all();
}

skuld::default_labels!(segment, display_set, subtitle);

fn sample_path(name: &str) -> PathBuf {
    let mut path = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    path.push("tests");
    path.push("data");
    path.push(name);
    path
}

// Segment-level tests =================================================================================================

#[skuld::test(labels = [segment])]
fn read_all_segments_sample1() {
    let file = File::open(sample_path("sample-1.sup")).unwrap();
    let reader = SupReader::new(file);
    let segments: Vec<_> = reader.segments().collect::<Result<_, _>>().unwrap();
    assert!(!segments.is_empty(), "expected segments in sample-1.sup");
}

#[skuld::test(labels = [segment])]
fn read_all_segments_sample2() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let segments: Vec<_> = reader.segments().collect::<Result<_, _>>().unwrap();
    assert!(!segments.is_empty(), "expected segments in sample-2.sup");
}

#[skuld::test(labels = [segment])]
fn segment_types_present_in_sample2() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);

    let mut has_pcs = false;
    let mut has_wds = false;
    let mut has_pds = false;
    let mut has_ods = false;
    let mut has_end = false;

    for seg in reader.segments() {
        let seg = seg.unwrap();
        match &seg.body {
            SegmentBody::PresentationComposition(_) => has_pcs = true,
            SegmentBody::WindowDefinition(_) => has_wds = true,
            SegmentBody::PaletteDefinition(_) => has_pds = true,
            SegmentBody::ObjectDefinition(_) => has_ods = true,
            SegmentBody::End => has_end = true,
        }
    }

    assert!(has_pcs, "missing PCS");
    assert!(has_wds, "missing WDS");
    assert!(has_pds, "missing PDS");
    assert!(has_ods, "missing ODS");
    assert!(has_end, "missing END");
}

#[skuld::test(labels = [segment])]
fn pcs_segments_have_valid_dimensions() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);

    for seg in reader.segments() {
        let seg = seg.unwrap();
        if let SegmentBody::PresentationComposition(pcs) = &seg.body {
            assert!(pcs.width > 0, "PCS width should be > 0");
            assert!(pcs.height > 0, "PCS height should be > 0");
        }
    }
}

// Display set tests ===================================================================================================

#[skuld::test(labels = [display_set])]
fn display_sets_sample2_nonempty() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let display_sets: Vec<_> = reader.display_sets().collect::<Result<_, _>>().unwrap();
    assert!(!display_sets.is_empty(), "expected display sets");
}

#[skuld::test(labels = [display_set])]
fn display_sets_timestamps_nondecreasing() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let display_sets: Vec<_> = reader.display_sets().collect::<Result<_, _>>().unwrap();

    for window in display_sets.windows(2) {
        assert!(
            window[1].timestamp >= window[0].timestamp,
            "timestamps should be non-decreasing: {} >= {}",
            window[1].timestamp,
            window[0].timestamp,
        );
    }
}

#[skuld::test(labels = [display_set])]
fn display_sets_last_has_unknown_duration() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let display_sets: Vec<_> = reader.display_sets().collect::<Result<_, _>>().unwrap();

    let last = display_sets.last().unwrap();
    assert_eq!(
        last.duration, UNKNOWN_DURATION,
        "last DS should have unknown duration"
    );

    // Non-last should have known duration
    for ds in &display_sets[..display_sets.len() - 1] {
        assert_ne!(
            ds.duration, UNKNOWN_DURATION,
            "non-last DS should have known duration"
        );
    }
}

#[skuld::test(labels = [display_set])]
fn display_sets_have_objects_and_palettes() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);

    let mut any_has_objects = false;
    let mut any_has_palettes = false;

    for ds in reader.display_sets() {
        let ds = ds.unwrap();
        if !ds.objects.is_empty() {
            any_has_objects = true;
        }
        if !ds.palettes.is_empty() {
            any_has_palettes = true;
        }
    }

    assert!(any_has_objects, "expected at least one DS with objects");
    assert!(any_has_palettes, "expected at least one DS with palettes");
}

// Subtitle tests ======================================================================================================

#[skuld::test(labels = [subtitle])]
fn subtitles_sample2() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let subtitles: Vec<_> = reader.subtitles().collect::<Result<_, _>>().unwrap();

    assert!(!subtitles.is_empty(), "expected subtitles");

    for sub in &subtitles {
        assert!(
            sub.bitmap.width() > 0,
            "subtitle bitmap width should be > 0"
        );
        assert!(
            sub.bitmap.height() > 0,
            "subtitle bitmap height should be > 0"
        );
        assert_eq!(
            sub.bitmap.data().len(),
            sub.bitmap.width() as usize * sub.bitmap.height() as usize,
            "bitmap data length should match dimensions"
        );
    }
}

#[skuld::test(labels = [subtitle])]
fn subtitles_have_nonzero_positions() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let subtitles: Vec<_> = reader.subtitles().collect::<Result<_, _>>().unwrap();

    // At least some subtitles should have non-zero y position (typically bottom of screen)
    let any_nonzero_y = subtitles.iter().any(|s| s.y > 0);
    assert!(any_nonzero_y, "expected some subtitles with y > 0");
}

#[skuld::test(labels = [subtitle])]
fn subtitles_timestamps_nondecreasing() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let subtitles: Vec<_> = reader.subtitles().collect::<Result<_, _>>().unwrap();

    for window in subtitles.windows(2) {
        assert!(
            window[1].timestamp >= window[0].timestamp,
            "subtitle timestamps should be non-decreasing"
        );
    }
}

#[skuld::test(labels = [subtitle])]
fn subtitles_sample1() {
    let file = File::open(sample_path("sample-1.sup")).unwrap();
    let reader = SupReader::new(file);
    let subtitles: Vec<_> = reader.subtitles().collect::<Result<_, _>>().unwrap();

    assert!(!subtitles.is_empty(), "expected subtitles in sample-1.sup");
}
