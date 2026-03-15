use pyo3::prelude::*;
use pyo3::types::PyType;

#[pyclass(name = "Timestamp", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyTimestamp {
    pub inner: pgs::Timestamp,
}

#[pymethods]
impl PyTimestamp {
    #[new]
    #[pyo3(signature = (*, ticks))]
    fn new(ticks: u32) -> Self {
        Self {
            inner: pgs::Timestamp::from_ticks(ticks),
        }
    }

    #[classmethod]
    fn from_millis(_cls: &Bound<'_, PyType>, ms: u32) -> Self {
        Self {
            inner: pgs::Timestamp::from_ticks(ms * 90),
        }
    }

    #[getter]
    fn ticks(&self) -> u32 {
        self.inner.as_ticks()
    }

    #[getter]
    fn millis(&self) -> u32 {
        self.inner.as_millis()
    }

    fn __repr__(&self) -> String {
        format!("Timestamp({})", self.inner)
    }

    fn __str__(&self) -> String {
        self.inner.to_string()
    }

    fn __eq__(&self, other: &PyTimestamp) -> bool {
        self.inner == other.inner
    }

    fn __ne__(&self, other: &PyTimestamp) -> bool {
        self.inner != other.inner
    }

    fn __lt__(&self, other: &PyTimestamp) -> bool {
        self.inner < other.inner
    }

    fn __le__(&self, other: &PyTimestamp) -> bool {
        self.inner <= other.inner
    }

    fn __gt__(&self, other: &PyTimestamp) -> bool {
        self.inner > other.inner
    }

    fn __ge__(&self, other: &PyTimestamp) -> bool {
        self.inner >= other.inner
    }

    fn __hash__(&self) -> u64 {
        self.inner.as_ticks() as u64
    }

    fn __add__(&self, other: &PyDuration) -> PyTimestamp {
        PyTimestamp {
            inner: self.inner + other.inner,
        }
    }

    fn __sub__(&self, py: Python<'_>, other: &Bound<'_, PyAny>) -> PyResult<PyObject> {
        if let Ok(ts) = other.extract::<PyRef<'_, PyTimestamp>>() {
            Ok(PyDuration {
                inner: self.inner - ts.inner,
            }
            .into_pyobject(py)?
            .into_any()
            .unbind())
        } else if let Ok(d) = other.extract::<PyRef<'_, PyDuration>>() {
            Ok(PyTimestamp {
                inner: self.inner - d.inner,
            }
            .into_pyobject(py)?
            .into_any()
            .unbind())
        } else {
            Err(pyo3::exceptions::PyTypeError::new_err(
                "can only subtract Timestamp or Duration from Timestamp",
            ))
        }
    }
}

#[pyclass(name = "Duration", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyDuration {
    pub inner: pgs::Duration,
}

#[pymethods]
impl PyDuration {
    #[new]
    #[pyo3(signature = (*, ticks))]
    fn new(ticks: u32) -> Self {
        Self {
            inner: pgs::Duration::from_ticks(ticks),
        }
    }

    #[classmethod]
    fn from_millis(_cls: &Bound<'_, PyType>, ms: u32) -> Self {
        Self {
            inner: pgs::Duration::from_millis(ms),
        }
    }

    #[classmethod]
    #[pyo3(name = "ZERO")]
    fn zero(_cls: &Bound<'_, PyType>) -> Self {
        Self {
            inner: pgs::Duration::ZERO,
        }
    }

    #[getter]
    fn ticks(&self) -> u32 {
        self.inner.as_ticks()
    }

    #[getter]
    fn millis(&self) -> u32 {
        self.inner.as_millis()
    }

    fn as_timedelta(&self, py: Python<'_>) -> PyResult<PyObject> {
        let ms = self.inner.as_std().as_millis() as i64;
        let datetime = py.import("datetime")?;
        let kwargs = pyo3::types::PyDict::new(py);
        kwargs.set_item("milliseconds", ms)?;
        let td = datetime.getattr("timedelta")?.call((), Some(&kwargs))?;
        Ok(td.unbind())
    }

    fn __repr__(&self) -> String {
        format!("Duration({})", self.inner)
    }

    fn __str__(&self) -> String {
        self.inner.to_string()
    }

    fn __eq__(&self, other: &PyDuration) -> bool {
        self.inner == other.inner
    }

    fn __ne__(&self, other: &PyDuration) -> bool {
        self.inner != other.inner
    }

    fn __lt__(&self, other: &PyDuration) -> bool {
        self.inner < other.inner
    }

    fn __le__(&self, other: &PyDuration) -> bool {
        self.inner <= other.inner
    }

    fn __gt__(&self, other: &PyDuration) -> bool {
        self.inner > other.inner
    }

    fn __ge__(&self, other: &PyDuration) -> bool {
        self.inner >= other.inner
    }

    fn __hash__(&self) -> u64 {
        self.inner.as_ticks() as u64
    }

    fn __add__(&self, other: &PyDuration) -> PyDuration {
        PyDuration {
            inner: pgs::Duration::from_ticks(self.inner.as_ticks() + other.inner.as_ticks()),
        }
    }

    fn __sub__(&self, other: &PyDuration) -> PyDuration {
        PyDuration {
            inner: pgs::Duration::from_ticks(
                self.inner.as_ticks().saturating_sub(other.inner.as_ticks()),
            ),
        }
    }
}

impl From<pgs::Timestamp> for PyTimestamp {
    fn from(ts: pgs::Timestamp) -> Self {
        Self { inner: ts }
    }
}

impl From<pgs::Duration> for PyDuration {
    fn from(d: pgs::Duration) -> Self {
        Self { inner: d }
    }
}
