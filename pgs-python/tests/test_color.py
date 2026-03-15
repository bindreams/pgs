import pgs


class TestRgba:
    def test_constructor(self):
        c = pgs.Rgba(10, 20, 30, 255)
        assert c.r == 10
        assert c.g == 20
        assert c.b == 30
        assert c.a == 255

    def test_to_tuple(self):
        c = pgs.Rgba(1, 2, 3, 4)
        assert c.to_tuple() == (1, 2, 3, 4)

    def test_eq_and_hash(self):
        a = pgs.Rgba(1, 2, 3, 4)
        b = pgs.Rgba(1, 2, 3, 4)
        c = pgs.Rgba(5, 6, 7, 8)
        assert a == b
        assert a != c
        assert hash(a) == hash(b)

    def test_repr(self):
        c = pgs.Rgba(10, 20, 30, 40)
        r = repr(c)
        assert "Rgba" in r
        assert "10" in r

    def test_roundtrip_via_ycbcra(self):
        # Pure white
        white = pgs.Rgba(255, 255, 255, 255)
        ycbcra = white.to_ycbcra()
        back = ycbcra.to_rgba()
        # Allow small rounding tolerance
        assert abs(back.r - 255) <= 2
        assert abs(back.g - 255) <= 2
        assert abs(back.b - 255) <= 2
        assert back.a == 255


class TestYCbCrA:
    def test_constructor(self):
        c = pgs.YCbCrA(16, 128, 128, 255)
        assert c.y == 16
        assert c.cr == 128
        assert c.cb == 128
        assert c.alpha == 255

    def test_to_tuple(self):
        c = pgs.YCbCrA(16, 128, 128, 255)
        assert c.to_tuple() == (16, 128, 128, 255)

    def test_to_rgba(self):
        c = pgs.YCbCrA(16, 128, 128, 255)
        rgba = c.to_rgba()
        assert isinstance(rgba, pgs.Rgba)
        assert rgba.a == 255

    def test_repr(self):
        c = pgs.YCbCrA(16, 128, 128, 255)
        assert "YCbCrA" in repr(c)
