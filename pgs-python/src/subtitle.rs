use pyo3::prelude::*;

use crate::bitmap::PyBitmap;
use crate::time::{PyDuration, PyTimestamp};

#[pyclass(name = "Subtitle", module = "pgs", frozen)]
pub struct PySubtitle {
    bitmap: Py<PyBitmap>,
    x: u16,
    y: u16,
    timestamp: pgs::time::Timestamp,
    duration: pgs::time::Duration,
    width: u16,
    height: u16,
}

impl PySubtitle {
    pub fn from_subtitle(py: Python<'_>, sub: pgs::Subtitle) -> PyResult<Self> {
        let width = sub.bitmap.width();
        let height = sub.bitmap.height();
        let bitmap = Py::new(py, PyBitmap::from_rgba_bitmap(sub.bitmap))?;
        Ok(Self {
            bitmap,
            x: sub.x,
            y: sub.y,
            timestamp: sub.timestamp,
            duration: sub.duration,
            width,
            height,
        })
    }
}

#[pymethods]
impl PySubtitle {
    #[getter]
    fn bitmap(&self, py: Python<'_>) -> Py<PyBitmap> {
        self.bitmap.clone_ref(py)
    }

    #[getter]
    fn x(&self) -> u16 {
        self.x
    }

    #[getter]
    fn y(&self) -> u16 {
        self.y
    }

    #[getter]
    fn timestamp(&self) -> PyTimestamp {
        self.timestamp.into()
    }

    #[getter]
    fn duration(&self) -> PyDuration {
        self.duration.into()
    }

    fn __repr__(&self) -> String {
        format!(
            "Subtitle({}x{} at ({}, {}), timestamp={})",
            self.width, self.height, self.x, self.y, self.timestamp
        )
    }
}
