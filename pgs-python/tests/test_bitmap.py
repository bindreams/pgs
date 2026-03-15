import pgs


class TestBitmapFromSubtitle:
    """Test Bitmap through the Subtitle API (no direct constructor exposed)."""

    def test_bitmap_properties(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        assert bmp.width > 0
        assert bmp.height > 0
        assert len(bmp) == bmp.width * bmp.height

    def test_bitmap_data_is_bytes(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        data = bmp.data
        assert isinstance(data, bytes)
        assert len(data) == bmp.width * bmp.height * 4  # RGBA

    def test_bitmap_get_pixel(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        px = bmp.get(0, 0)
        assert isinstance(px, pgs.Rgba)

    def test_bitmap_row(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        row = bmp.row(0)
        assert isinstance(row, bytes)
        assert len(row) == bmp.width * 4

    def test_bitmap_repr(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        assert "Bitmap" in repr(bmp)

    def test_bitmap_buffer_protocol(self, sample2_path):
        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        mv = memoryview(bmp)
        assert mv.readonly
        assert len(mv) == bmp.width * bmp.height * 4

    def test_bitmap_out_of_bounds(self, sample2_path):
        import pytest

        reader = pgs.SupReader(sample2_path)
        sub = next(iter(reader.subtitles()))
        bmp = sub.bitmap

        with pytest.raises(IndexError):
            bmp.get(bmp.width, 0)
        with pytest.raises(IndexError):
            bmp.row(bmp.height)
