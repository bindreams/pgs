use super::*;

#[skuld::test]
fn duration_from_millis_roundtrip() {
    let d = Duration::from_millis(1234);
    assert_eq!(d.as_millis(), 1234);
}

#[skuld::test]
fn duration_ticks_roundtrip() {
    let d = Duration::from_ticks(90_000);
    assert_eq!(d.as_ticks(), 90_000);
    // 90000 ticks = 1 second = 1000 ms
    assert_eq!(d.as_millis(), 1000);
}

#[skuld::test]
fn duration_as_std() {
    let d = Duration::from_ticks(90_000);
    let std_d = d.as_std();
    assert_eq!(std_d, std::time::Duration::from_secs(1));
}

#[skuld::test]
fn duration_display() {
    let d = Duration::from_millis(3_723_456);
    // 3723456 ms = 1h 2m 3s 456ms
    assert_eq!(d.to_string(), "01:02:03.456");
}

#[skuld::test]
fn duration_display_unknown() {
    assert_eq!(UNKNOWN_DURATION.to_string(), "??:??:??.???");
}

#[skuld::test]
fn timestamp_sub() {
    let a = Timestamp::from_ticks(180_000);
    let b = Timestamp::from_ticks(90_000);
    let d = a - b;
    assert_eq!(d, Duration::from_ticks(90_000));
}

#[skuld::test]
fn timestamp_add_duration() {
    let ts = Timestamp::from_ticks(90_000);
    let d = Duration::from_ticks(45_000);
    assert_eq!((ts + d).as_ticks(), 135_000);
}

#[skuld::test]
fn timestamp_sub_duration() {
    let ts = Timestamp::from_ticks(90_000);
    let d = Duration::from_ticks(45_000);
    assert_eq!((ts - d).as_ticks(), 45_000);
}

#[skuld::test]
fn timestamp_display() {
    let ts = Timestamp::from_ticks(90_000 * 3661); // 1h 1m 1s
    assert_eq!(ts.to_string(), "01:01:01.000");
}

#[skuld::test]
fn timestamp_sub_saturates() {
    let a = Timestamp::from_ticks(100);
    let b = Timestamp::from_ticks(200);
    assert_eq!((a - b), Duration::from_ticks(0));
}
