use std::fmt;
use std::ops::{Add, Sub};

/// A duration in the PGS 90 kHz clock.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Duration {
    ticks: u32,
}

/// Sentinel value representing an unknown duration (e.g. the last subtitle in a stream).
pub const UNKNOWN_DURATION: Duration = Duration { ticks: u32::MAX };

impl Duration {
    pub const ZERO: Self = Self { ticks: 0 };

    pub const fn from_ticks(ticks: u32) -> Self {
        Self { ticks }
    }

    pub const fn as_ticks(self) -> u32 {
        self.ticks
    }

    pub const fn from_millis(ms: u32) -> Self {
        Self { ticks: ms * 90 }
    }

    pub const fn as_millis(self) -> u32 {
        self.ticks / 90
    }

    pub fn as_std(self) -> std::time::Duration {
        std::time::Duration::from_nanos(self.ticks as u64 * 1_000_000_000 / 90_000)
    }
}

impl fmt::Display for Duration {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if *self == UNKNOWN_DURATION {
            return write!(f, "??:??:??.???");
        }
        let total_ms = self.as_millis();
        let ms = total_ms % 1000;
        let total_s = total_ms / 1000;
        let s = total_s % 60;
        let total_m = total_s / 60;
        let m = total_m % 60;
        let h = total_m / 60;
        write!(f, "{h:02}:{m:02}:{s:02}.{ms:03}")
    }
}

/// A presentation timestamp in the PGS 90 kHz clock.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Timestamp {
    ticks: u32,
}

impl Timestamp {
    pub const fn from_ticks(ticks: u32) -> Self {
        Self { ticks }
    }

    pub const fn as_ticks(self) -> u32 {
        self.ticks
    }

    pub const fn as_millis(self) -> u32 {
        self.ticks / 90
    }
}

impl fmt::Display for Timestamp {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let total_ms = self.as_millis();
        let ms = total_ms % 1000;
        let total_s = total_ms / 1000;
        let s = total_s % 60;
        let total_m = total_s / 60;
        let m = total_m % 60;
        let h = total_m / 60;
        write!(f, "{h:02}:{m:02}:{s:02}.{ms:03}")
    }
}

impl Sub for Timestamp {
    type Output = Duration;
    fn sub(self, rhs: Self) -> Duration {
        Duration::from_ticks(self.ticks.saturating_sub(rhs.ticks))
    }
}

impl Add<Duration> for Timestamp {
    type Output = Timestamp;
    fn add(self, rhs: Duration) -> Timestamp {
        Timestamp::from_ticks(self.ticks.saturating_add(rhs.ticks))
    }
}

impl Sub<Duration> for Timestamp {
    type Output = Timestamp;
    fn sub(self, rhs: Duration) -> Timestamp {
        Timestamp::from_ticks(self.ticks.saturating_sub(rhs.ticks))
    }
}

#[cfg(test)]
#[path = "time_tests.rs"]
mod tests;
