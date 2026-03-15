mod bitmap;
mod color;
mod display_set;
mod error;
mod py_reader;
mod reader;
mod segment;
mod subtitle;
mod time;

use pyo3::prelude::*;

use reader::PySupReader;
use time::PyDuration;

/// Python bindings for the pgs PGS subtitle parser.
#[pymodule]
fn pgs(m: &Bound<'_, PyModule>) -> PyResult<()> {
    // Exceptions
    error::register(m)?;

    // Time
    m.add_class::<time::PyTimestamp>()?;
    m.add_class::<time::PyDuration>()?;
    m.add(
        "UNKNOWN_DURATION",
        PyDuration::from(::pgs::time::UNKNOWN_DURATION),
    )?;

    // Color
    m.add_class::<color::PyRgba>()?;
    m.add_class::<color::PyYCbCrA>()?;

    // Bitmap
    m.add_class::<bitmap::PyBitmap>()?;

    // Segment types
    m.add_class::<segment::PySegment>()?;
    m.add_class::<segment::PySegmentBody>()?;
    m.add_class::<segment::PyCompositionState>()?;
    m.add_class::<segment::PyPresentationComposition>()?;
    m.add_class::<segment::PyCompositionObject>()?;
    m.add_class::<segment::PyCropRect>()?;
    m.add_class::<segment::PyWindowDefinition>()?;
    m.add_class::<segment::PyWindow>()?;
    m.add_class::<segment::PyPaletteDefinition>()?;
    m.add_class::<segment::PyPaletteEntry>()?;
    m.add_class::<segment::PyObjectDefinition>()?;

    // DisplaySet
    m.add_class::<display_set::PyDisplaySet>()?;

    // Subtitle
    m.add_class::<subtitle::PySubtitle>()?;

    // Reader and iterators
    m.add_class::<reader::PySupReader>()?;
    m.add_class::<reader::PySegmentIter>()?;
    m.add_class::<reader::PyDisplaySetIter>()?;
    m.add_class::<reader::PySubtitleIter>()?;

    // Convenience function
    m.add_function(wrap_pyfunction!(open, m)?)?;

    Ok(())
}

/// Open a PGS subtitle file for reading.
///
/// Shorthand for ``SupReader(path)``.
#[pyfunction]
fn open(source: &Bound<'_, PyAny>) -> PyResult<PySupReader> {
    PySupReader::new(source)
}
