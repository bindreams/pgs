import datetime

import pgs


class TestTimestamp:
    def test_from_ticks(self):
        ts = pgs.Timestamp(ticks=90_000)
        assert ts.ticks == 90_000
        assert ts.millis == 1000

    def test_from_millis(self):
        ts = pgs.Timestamp.from_millis(500)
        assert ts.millis == 500
        assert ts.ticks == 500 * 90

    def test_repr(self):
        ts = pgs.Timestamp(ticks=90_000)
        assert "Timestamp" in repr(ts)

    def test_str(self):
        ts = pgs.Timestamp(ticks=90_000)
        assert str(ts) == "00:00:01.000"

    def test_comparison(self):
        a = pgs.Timestamp(ticks=100)
        b = pgs.Timestamp(ticks=200)
        assert a < b
        assert a <= b
        assert b > a
        assert b >= a
        assert a == pgs.Timestamp(ticks=100)
        assert a != b

    def test_hash(self):
        a = pgs.Timestamp(ticks=100)
        b = pgs.Timestamp(ticks=100)
        assert hash(a) == hash(b)
        assert {a, b} == {a}

    def test_sub_timestamps(self):
        a = pgs.Timestamp(ticks=200)
        b = pgs.Timestamp(ticks=100)
        d = a - b
        assert isinstance(d, pgs.Duration)
        assert d.ticks == 100

    def test_add_duration(self):
        ts = pgs.Timestamp(ticks=100)
        d = pgs.Duration(ticks=50)
        result = ts + d
        assert isinstance(result, pgs.Timestamp)
        assert result.ticks == 150

    def test_sub_duration(self):
        ts = pgs.Timestamp(ticks=100)
        d = pgs.Duration(ticks=30)
        result = ts - d
        assert isinstance(result, pgs.Timestamp)
        assert result.ticks == 70


class TestDuration:
    def test_from_ticks(self):
        d = pgs.Duration(ticks=9000)
        assert d.ticks == 9000
        assert d.millis == 100

    def test_from_millis(self):
        d = pgs.Duration.from_millis(250)
        assert d.millis == 250
        assert d.ticks == 250 * 90

    def test_zero(self):
        d = pgs.Duration.ZERO()
        assert d.ticks == 0

    def test_comparison(self):
        a = pgs.Duration(ticks=100)
        b = pgs.Duration(ticks=200)
        assert a < b
        assert a != b

    def test_add(self):
        a = pgs.Duration(ticks=100)
        b = pgs.Duration(ticks=200)
        c = a + b
        assert c.ticks == 300

    def test_sub(self):
        a = pgs.Duration(ticks=200)
        b = pgs.Duration(ticks=100)
        c = a - b
        assert c.ticks == 100

    def test_as_timedelta(self):
        d = pgs.Duration.from_millis(1500)
        td = d.as_timedelta()
        assert isinstance(td, datetime.timedelta)
        assert td == datetime.timedelta(milliseconds=1500)

    def test_repr(self):
        d = pgs.Duration(ticks=9000)
        assert "Duration" in repr(d)


class TestUnknownDuration:
    def test_unknown_duration_exists(self):
        d = pgs.UNKNOWN_DURATION
        assert isinstance(d, pgs.Duration)

    def test_unknown_duration_str(self):
        assert "??" in str(pgs.UNKNOWN_DURATION)
