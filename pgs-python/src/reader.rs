use std::fs::File;
use std::io::BufReader;

use pyo3::prelude::*;
use pyo3::types::PyString;

use crate::display_set::PyDisplaySet;
use crate::error::{consumed_err, to_py_err};
use crate::py_reader::PyReader;
use crate::segment::PySegment;
use crate::subtitle::PySubtitle;

// Type-erased iterator enums ==========================================================================================

enum AnySegments {
    File(pgs::read::Segments<BufReader<File>>),
    PyObject(pgs::read::Segments<PyReader>),
}

impl AnySegments {
    fn next_item(&mut self) -> Option<Result<pgs::Segment, pgs::Error>> {
        match self {
            Self::File(it) => it.next(),
            Self::PyObject(it) => it.next(),
        }
    }
}

enum AnyDisplaySets {
    File(pgs::read::DisplaySets<BufReader<File>>),
    PyObject(pgs::read::DisplaySets<PyReader>),
}

impl AnyDisplaySets {
    fn next_item(&mut self) -> Option<Result<pgs::DisplaySet, pgs::Error>> {
        match self {
            Self::File(it) => it.next(),
            Self::PyObject(it) => it.next(),
        }
    }
}

enum AnySubtitles {
    File(pgs::read::Subtitles<BufReader<File>>),
    PyObject(pgs::read::Subtitles<PyReader>),
}

impl AnySubtitles {
    fn next_item(&mut self) -> Option<Result<pgs::Subtitle, pgs::Error>> {
        match self {
            Self::File(it) => it.next(),
            Self::PyObject(it) => it.next(),
        }
    }
}

// SupReader ===========================================================================================================

enum ReaderInner {
    File(pgs::SupReader<BufReader<File>>),
    PyObject(pgs::SupReader<PyReader>),
}

#[pyclass(name = "SupReader", module = "pgs")]
pub struct PySupReader {
    inner: Option<ReaderInner>,
}

#[pymethods]
impl PySupReader {
    #[new]
    pub fn new(source: &Bound<'_, PyAny>) -> PyResult<Self> {
        // Try os.PathLike first
        let py = source.py();
        let path_str = if let Ok(s) = source.downcast::<PyString>() {
            Some(s.to_string())
        } else if let Ok(fspath) = py.import("os")?.call_method1("fspath", (source,)) {
            Some(fspath.extract::<String>()?)
        } else {
            None
        };

        if let Some(path) = path_str {
            let file = File::open(&path).map_err(|e| {
                pyo3::exceptions::PyIOError::new_err(format!("cannot open '{path}': {e}"))
            })?;
            let reader = pgs::SupReader::new(BufReader::new(file));
            Ok(Self {
                inner: Some(ReaderInner::File(reader)),
            })
        } else {
            // Assume file-like object with .read() method
            if !source.hasattr("read")? {
                return Err(pyo3::exceptions::PyTypeError::new_err(
                    "expected a file path (str/PathLike) or a file-like object with a .read() method",
                ));
            }
            let obj = source.clone().unbind();
            let reader = pgs::SupReader::new(PyReader::new(obj));
            Ok(Self {
                inner: Some(ReaderInner::PyObject(reader)),
            })
        }
    }

    fn segments(&mut self) -> PyResult<PySegmentIter> {
        let inner = self.inner.take().ok_or_else(consumed_err)?;
        let iter = match inner {
            ReaderInner::File(r) => AnySegments::File(r.segments()),
            ReaderInner::PyObject(r) => AnySegments::PyObject(r.segments()),
        };
        Ok(PySegmentIter { inner: iter })
    }

    fn display_sets(&mut self) -> PyResult<PyDisplaySetIter> {
        let inner = self.inner.take().ok_or_else(consumed_err)?;
        let iter = match inner {
            ReaderInner::File(r) => AnyDisplaySets::File(r.display_sets()),
            ReaderInner::PyObject(r) => AnyDisplaySets::PyObject(r.display_sets()),
        };
        Ok(PyDisplaySetIter { inner: iter })
    }

    fn subtitles(&mut self) -> PyResult<PySubtitleIter> {
        let inner = self.inner.take().ok_or_else(consumed_err)?;
        let iter = match inner {
            ReaderInner::File(r) => AnySubtitles::File(r.subtitles()),
            ReaderInner::PyObject(r) => AnySubtitles::PyObject(r.subtitles()),
        };
        Ok(PySubtitleIter { inner: iter })
    }

    fn __repr__(&self) -> String {
        if self.inner.is_some() {
            "SupReader(<ready>)".to_string()
        } else {
            "SupReader(<consumed>)".to_string()
        }
    }
}

// Iterator wrappers ===================================================================================================

#[pyclass(name = "SegmentIter", module = "pgs")]
pub struct PySegmentIter {
    inner: AnySegments,
}

#[pymethods]
impl PySegmentIter {
    fn __iter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    fn __next__(&mut self) -> PyResult<Option<PySegment>> {
        match self.inner.next_item() {
            Some(Ok(seg)) => Ok(Some(PySegment::from(seg))),
            Some(Err(e)) => Err(to_py_err(e)),
            None => Ok(None),
        }
    }
}

#[pyclass(name = "DisplaySetIter", module = "pgs")]
pub struct PyDisplaySetIter {
    inner: AnyDisplaySets,
}

#[pymethods]
impl PyDisplaySetIter {
    fn __iter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    fn __next__(&mut self) -> PyResult<Option<PyDisplaySet>> {
        match self.inner.next_item() {
            Some(Ok(ds)) => Ok(Some(PyDisplaySet::from_display_set(ds))),
            Some(Err(e)) => Err(to_py_err(e)),
            None => Ok(None),
        }
    }
}

#[pyclass(name = "SubtitleIter", module = "pgs")]
pub struct PySubtitleIter {
    inner: AnySubtitles,
}

#[pymethods]
impl PySubtitleIter {
    fn __iter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    fn __next__(&mut self, py: Python<'_>) -> PyResult<Option<PySubtitle>> {
        match self.inner.next_item() {
            Some(Ok(sub)) => Ok(Some(PySubtitle::from_subtitle(py, sub)?)),
            Some(Err(e)) => Err(to_py_err(e)),
            None => Ok(None),
        }
    }
}
