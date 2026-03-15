from pathlib import Path

import pytest

DATA_DIR = Path(__file__).parent.parent.parent / "tests" / "data"


@pytest.fixture
def sample1_path() -> Path:
    return DATA_DIR / "sample-1.sup"


@pytest.fixture
def sample2_path() -> Path:
    return DATA_DIR / "sample-2.sup"
