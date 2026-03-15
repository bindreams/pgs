import io
from pathlib import Path

import pytest

import pgs


class TestSupReaderConstructor:
    def test_open_str_path(self, sample2_path):
        reader = pgs.SupReader(str(sample2_path))
        assert repr(reader) == "SupReader(<ready>)"

    def test_open_pathlib_path(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        assert repr(reader) == "SupReader(<ready>)"

    def test_open_file_object(self, sample2_path):
        with open(sample2_path, "rb") as f:
            reader = pgs.SupReader(f)
            assert repr(reader) == "SupReader(<ready>)"

    def test_open_bytes_io(self, sample2_path):
        data = sample2_path.read_bytes()
        reader = pgs.SupReader(io.BytesIO(data))
        assert repr(reader) == "SupReader(<ready>)"

    def test_open_nonexistent_file(self):
        with pytest.raises(IOError):
            pgs.SupReader("nonexistent_file.sup")

    def test_invalid_source(self):
        with pytest.raises(TypeError):
            pgs.SupReader(42)


class TestSupReaderConsumed:
    def test_consumed_after_segments(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        reader.segments()
        assert repr(reader) == "SupReader(<consumed>)"
        with pytest.raises(ValueError, match="consumed"):
            reader.segments()

    def test_consumed_after_display_sets(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        reader.display_sets()
        with pytest.raises(ValueError, match="consumed"):
            reader.subtitles()


class TestConvenienceOpen:
    def test_open_function(self, sample2_path):
        reader = pgs.open(sample2_path)
        assert isinstance(reader, pgs.SupReader)


class TestSegments:
    def test_iterate_segments(self, sample2_path):
        segments = list(pgs.SupReader(sample2_path).segments())
        assert len(segments) > 0

    def test_segment_properties(self, sample2_path):
        seg = next(iter(pgs.SupReader(sample2_path).segments()))
        assert isinstance(seg.pts, pgs.Timestamp)
        assert isinstance(seg.dts, pgs.Timestamp)
        assert isinstance(seg.body, pgs.SegmentBody)

    def test_segment_types_present(self, sample2_path):
        kinds = set()
        for seg in pgs.SupReader(sample2_path).segments():
            kinds.add(seg.body.kind)
        assert "presentation_composition" in kinds
        assert "window_definition" in kinds
        assert "palette_definition" in kinds
        assert "object_definition" in kinds
        assert "end" in kinds

    def test_segment_body_accessors(self, sample2_path):
        for seg in pgs.SupReader(sample2_path).segments():
            body = seg.body
            if body.kind == "presentation_composition":
                pcs = body.as_presentation_composition()
                assert pcs is not None
                assert pcs.width > 0
                assert pcs.height > 0
                assert body.as_window_definition() is None
                break

    def test_end_segment(self, sample2_path):
        for seg in pgs.SupReader(sample2_path).segments():
            if seg.is_end():
                assert seg.body.kind == "end"
                break

    def test_iter_protocol(self, sample2_path):
        it = pgs.SupReader(sample2_path).segments()
        assert iter(it) is it


class TestDisplaySets:
    def test_iterate_display_sets(self, sample2_path):
        display_sets = list(pgs.SupReader(sample2_path).display_sets())
        assert len(display_sets) > 0

    def test_display_set_properties(self, sample2_path):
        ds = next(iter(pgs.SupReader(sample2_path).display_sets()))
        assert isinstance(ds.timestamp, pgs.Timestamp)
        assert isinstance(ds.duration, pgs.Duration)
        assert isinstance(ds.state, pgs.CompositionState)

    def test_display_set_windows(self, sample2_path):
        ds = next(iter(pgs.SupReader(sample2_path).display_sets()))
        windows = ds.windows
        assert isinstance(windows, dict)
        for wid, window in windows.items():
            assert isinstance(wid, int)
            assert isinstance(window, pgs.Window)
            assert window.width > 0

    def test_display_set_palettes(self, sample2_path):
        ds = next(iter(pgs.SupReader(sample2_path).display_sets()))
        palettes = ds.palettes
        assert isinstance(palettes, dict)

    def test_display_set_compositions(self, sample2_path):
        ds = next(iter(pgs.SupReader(sample2_path).display_sets()))
        comps = ds.compositions
        assert isinstance(comps, list)
        assert len(comps) > 0
        assert isinstance(comps[0], pgs.CompositionObject)

    def test_timestamps_nondecreasing(self, sample2_path):
        prev = None
        for ds in pgs.SupReader(sample2_path).display_sets():
            if prev is not None:
                assert ds.timestamp >= prev
            prev = ds.timestamp

    def test_last_has_unknown_duration(self, sample2_path):
        display_sets = list(pgs.SupReader(sample2_path).display_sets())
        assert display_sets[-1].duration == pgs.UNKNOWN_DURATION


class TestSubtitles:
    def test_iterate_subtitles(self, sample2_path):
        subtitles = list(pgs.SupReader(sample2_path).subtitles())
        assert len(subtitles) > 0

    def test_subtitle_properties(self, sample2_path):
        sub = next(iter(pgs.SupReader(sample2_path).subtitles()))
        assert isinstance(sub.bitmap, pgs.Bitmap)
        assert isinstance(sub.x, int)
        assert isinstance(sub.y, int)
        assert isinstance(sub.timestamp, pgs.Timestamp)
        assert isinstance(sub.duration, pgs.Duration)

    def test_subtitle_bitmap_has_data(self, sample2_path):
        sub = next(iter(pgs.SupReader(sample2_path).subtitles()))
        assert sub.bitmap.width > 0
        assert sub.bitmap.height > 0
        assert len(sub.bitmap.data) > 0

    def test_subtitles_timestamps_nondecreasing(self, sample2_path):
        prev = None
        for sub in pgs.SupReader(sample2_path).subtitles():
            if prev is not None:
                assert sub.timestamp >= prev
            prev = sub.timestamp

    def test_subtitles_repr(self, sample2_path):
        sub = next(iter(pgs.SupReader(sample2_path).subtitles()))
        assert "Subtitle" in repr(sub)

    def test_sample1(self, sample1_path):
        subtitles = list(pgs.SupReader(sample1_path).subtitles())
        assert len(subtitles) > 0

    def test_file_like_object(self, sample2_path):
        with open(sample2_path, "rb") as f:
            subtitles = list(pgs.SupReader(f).subtitles())
            assert len(subtitles) > 0

    def test_bytes_io(self, sample2_path):
        import io
        data = sample2_path.read_bytes()
        subtitles = list(pgs.SupReader(io.BytesIO(data)).subtitles())
        assert len(subtitles) > 0
