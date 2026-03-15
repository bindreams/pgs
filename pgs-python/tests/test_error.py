import io

import pytest

import pgs


class TestErrors:
    def test_pgs_error_is_runtime_error(self):
        assert issubclass(pgs.PgsError, RuntimeError)

    def test_pgs_read_error_is_pgs_error(self):
        assert issubclass(pgs.PgsReadError, pgs.PgsError)

    def test_pgs_rle_error_is_pgs_error(self):
        assert issubclass(pgs.PgsRleError, pgs.PgsError)

    def test_invalid_data_raises(self):
        bad_data = io.BytesIO(b"not a PGS file at all")
        reader = pgs.SupReader(bad_data)
        with pytest.raises(pgs.PgsReadError):
            list(reader.segments())

    def test_truncated_data_raises(self):
        # A valid PGS magic with a segment type byte claiming a body that isn't there.
        # Full header is 13 bytes: "PG" + 4 PTS + 4 DTS + 1 type + 2 size.
        # We set size to 100 but provide no body bytes.
        header = b"PG" + b"\x00" * 8 + b"\x16" + b"\x00\x64"
        bad_data = io.BytesIO(header)
        reader = pgs.SupReader(bad_data)
        with pytest.raises((pgs.PgsReadError, IOError)):
            list(reader.segments())

    def test_empty_data_returns_nothing(self):
        reader = pgs.SupReader(io.BytesIO(b""))
        segments = list(reader.segments())
        assert segments == []
