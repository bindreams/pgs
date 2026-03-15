use pyo3::prelude::*;

#[pyclass(name = "Rgba", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyRgba {
    pub inner: pgs::Rgba,
}

#[pymethods]
impl PyRgba {
    #[new]
    fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self {
            inner: pgs::Rgba { r, g, b, a },
        }
    }

    #[getter]
    fn r(&self) -> u8 {
        self.inner.r
    }

    #[getter]
    fn g(&self) -> u8 {
        self.inner.g
    }

    #[getter]
    fn b(&self) -> u8 {
        self.inner.b
    }

    #[getter]
    fn a(&self) -> u8 {
        self.inner.a
    }

    fn to_tuple(&self) -> (u8, u8, u8, u8) {
        (self.inner.r, self.inner.g, self.inner.b, self.inner.a)
    }

    fn to_ycbcra(&self) -> PyYCbCrA {
        PyYCbCrA {
            inner: pgs::YCbCrA::from(self.inner),
        }
    }

    fn __repr__(&self) -> String {
        format!(
            "Rgba(r={}, g={}, b={}, a={})",
            self.inner.r, self.inner.g, self.inner.b, self.inner.a
        )
    }

    fn __eq__(&self, other: &PyRgba) -> bool {
        self.inner == other.inner
    }

    fn __hash__(&self) -> u64 {
        let pgs::Rgba { r, g, b, a } = self.inner;
        u32::from_be_bytes([r, g, b, a]) as u64
    }
}

#[pyclass(name = "YCbCrA", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyYCbCrA {
    pub inner: pgs::YCbCrA,
}

#[pymethods]
impl PyYCbCrA {
    #[new]
    fn new(y: u8, cr: u8, cb: u8, alpha: u8) -> Self {
        Self {
            inner: pgs::YCbCrA { y, cr, cb, alpha },
        }
    }

    #[getter]
    fn y(&self) -> u8 {
        self.inner.y
    }

    #[getter]
    fn cr(&self) -> u8 {
        self.inner.cr
    }

    #[getter]
    fn cb(&self) -> u8 {
        self.inner.cb
    }

    #[getter]
    fn alpha(&self) -> u8 {
        self.inner.alpha
    }

    fn to_rgba(&self) -> PyRgba {
        PyRgba {
            inner: pgs::Rgba::from(self.inner),
        }
    }

    fn to_tuple(&self) -> (u8, u8, u8, u8) {
        (self.inner.y, self.inner.cr, self.inner.cb, self.inner.alpha)
    }

    fn __repr__(&self) -> String {
        format!(
            "YCbCrA(y={}, cr={}, cb={}, alpha={})",
            self.inner.y, self.inner.cr, self.inner.cb, self.inner.alpha
        )
    }

    fn __eq__(&self, other: &PyYCbCrA) -> bool {
        self.inner == other.inner
    }

    fn __hash__(&self) -> u64 {
        let pgs::YCbCrA { y, cr, cb, alpha } = self.inner;
        u32::from_be_bytes([y, cr, cb, alpha]) as u64
    }
}

impl From<pgs::Rgba> for PyRgba {
    fn from(c: pgs::Rgba) -> Self {
        Self { inner: c }
    }
}

impl From<pgs::YCbCrA> for PyYCbCrA {
    fn from(c: pgs::YCbCrA) -> Self {
        Self { inner: c }
    }
}
