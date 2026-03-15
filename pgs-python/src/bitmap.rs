use pyo3::prelude::*;
use pyo3::types::PyBytes;

use crate::color::PyRgba;

#[pyclass(name = "Bitmap", module = "pgs")]
pub struct PyBitmap {
    width: u16,
    height: u16,
    /// Pre-computed RGBA bytes: [r, g, b, a, r, g, b, a, ...]
    bytes: Vec<u8>,
}

impl PyBitmap {
    pub fn from_rgba_bitmap(bmp: pgs::bitmap::Bitmap<pgs::Rgba>) -> Self {
        let width = bmp.width();
        let height = bmp.height();
        let data = bmp.into_data();
        let mut bytes = Vec::with_capacity(data.len() * 4);
        for px in &data {
            bytes.push(px.r);
            bytes.push(px.g);
            bytes.push(px.b);
            bytes.push(px.a);
        }
        Self {
            width,
            height,
            bytes,
        }
    }

    pub fn from_indexed_bitmap(bmp: &pgs::bitmap::Bitmap<u8>) -> Self {
        let width = bmp.width();
        let height = bmp.height();
        let bytes = bmp.data().to_vec();
        Self {
            width,
            height,
            bytes,
        }
    }
}

#[pymethods]
impl PyBitmap {
    #[getter]
    fn width(&self) -> u16 {
        self.width
    }

    #[getter]
    fn height(&self) -> u16 {
        self.height
    }

    #[getter]
    fn data<'py>(&self, py: Python<'py>) -> Bound<'py, PyBytes> {
        PyBytes::new(py, &self.bytes)
    }

    fn get(&self, x: u16, y: u16) -> PyResult<PyRgba> {
        if x >= self.width || y >= self.height {
            return Err(pyo3::exceptions::PyIndexError::new_err(format!(
                "pixel ({x}, {y}) out of range for {}x{} bitmap",
                self.width, self.height
            )));
        }
        let offset = (y as usize * self.width as usize + x as usize) * 4;
        Ok(PyRgba {
            inner: pgs::Rgba {
                r: self.bytes[offset],
                g: self.bytes[offset + 1],
                b: self.bytes[offset + 2],
                a: self.bytes[offset + 3],
            },
        })
    }

    fn row<'py>(&self, py: Python<'py>, y: u16) -> PyResult<Bound<'py, PyBytes>> {
        if y >= self.height {
            return Err(pyo3::exceptions::PyIndexError::new_err(format!(
                "row {y} out of range for bitmap with height {}",
                self.height
            )));
        }
        let start = y as usize * self.width as usize * 4;
        let end = start + self.width as usize * 4;
        Ok(PyBytes::new(py, &self.bytes[start..end]))
    }

    fn __len__(&self) -> usize {
        self.width as usize * self.height as usize
    }

    fn __repr__(&self) -> String {
        format!("Bitmap(width={}, height={})", self.width, self.height)
    }

    unsafe fn __getbuffer__(
        slf: Bound<'_, Self>,
        view: *mut pyo3::ffi::Py_buffer,
        flags: std::os::raw::c_int,
    ) -> PyResult<()> {
        unsafe {
            let this = slf.borrow();
            let buf = &this.bytes;

            if flags & pyo3::ffi::PyBUF_WRITABLE != 0 {
                return Err(pyo3::exceptions::PyBufferError::new_err(
                    "bitmap buffer is read-only",
                ));
            }

            let view = &mut *view;
            view.buf = buf.as_ptr() as *mut std::os::raw::c_void;
            view.len = buf.len() as isize;
            view.itemsize = 1;
            view.readonly = 1;
            view.ndim = 1;
            view.format = std::ptr::null_mut();
            if flags & pyo3::ffi::PyBUF_FORMAT != 0 {
                view.format = b"B\0".as_ptr() as *mut std::os::raw::c_char;
            }
            // Shape and strides are required for ndim >= 1.
            // We store them in `internal` as a heap-allocated pair.
            let layout = Box::into_raw(Box::new([buf.len() as isize, 1isize]));
            view.shape = layout as *mut isize;
            view.strides = layout.cast::<isize>().add(1);
            view.suboffsets = std::ptr::null_mut();
            view.internal = layout as *mut std::os::raw::c_void;

            view.obj = pyo3::ffi::Py_NewRef(slf.as_ptr());

            Ok(())
        }
    }

    unsafe fn __releasebuffer__(&self, view: *mut pyo3::ffi::Py_buffer) {
        unsafe {
            let view = &mut *view;
            if !view.internal.is_null() {
                drop(Box::from_raw(view.internal as *mut [isize; 2]));
                view.internal = std::ptr::null_mut();
            }
        }
    }
}
