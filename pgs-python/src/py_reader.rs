use std::io::{self, Read};

use pyo3::prelude::*;
use pyo3::types::PyBytes;

/// Adapter that implements `std::io::Read` by calling `.read(n)` on a Python
/// file-like object. Each call acquires the GIL.
pub struct PyReader {
    inner: Py<PyAny>,
}

impl PyReader {
    pub fn new(obj: Py<PyAny>) -> Self {
        Self { inner: obj }
    }
}

impl Read for PyReader {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        Python::with_gil(|py| {
            let result = self
                .inner
                .call_method1(py, "read", (buf.len(),))
                .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;

            let bytes: &Bound<'_, PyBytes> = result
                .downcast_bound::<PyBytes>(py)
                .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e.to_string()))?;

            let data = bytes.as_bytes();
            let n = data.len().min(buf.len());
            buf[..n].copy_from_slice(&data[..n]);
            Ok(n)
        })
    }
}
