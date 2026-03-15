use crate::SupReader;
use crate::time::UNKNOWN_DURATION;
use std::fs::File;

fn sample_path(name: &str) -> std::path::PathBuf {
    let mut path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    path.push("tests");
    path.push("data");
    path.push(name);
    path
}

#[skuld::test]
fn display_sets_sample2() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let mut count = 0;
    let mut prev_ts = None;

    for ds in reader.display_sets() {
        let ds = ds.unwrap();
        count += 1;

        // Timestamps should be non-decreasing
        if let Some(prev) = prev_ts {
            assert!(ds.timestamp >= prev, "timestamps should be non-decreasing");
        }
        prev_ts = Some(ds.timestamp);
    }

    assert!(count > 0, "expected at least one display set");
}

#[skuld::test]
fn display_sets_have_durations() {
    let file = File::open(sample_path("sample-2.sup")).unwrap();
    let reader = SupReader::new(file);
    let display_sets: Vec<_> = reader.display_sets().collect::<Result<_, _>>().unwrap();

    // All except the last should have a known duration
    for ds in &display_sets[..display_sets.len() - 1] {
        assert_ne!(
            ds.duration, UNKNOWN_DURATION,
            "non-last DS should have known duration"
        );
    }

    // Last display set gets UNKNOWN_DURATION
    assert_eq!(display_sets.last().unwrap().duration, UNKNOWN_DURATION);
}
