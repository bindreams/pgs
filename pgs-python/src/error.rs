use pyo3::exceptions::{PyIOError, PyRuntimeError, PyValueError};
use pyo3::prelude::*;

pyo3::create_exception!(
    pgs,
    PgsError,
    PyRuntimeError,
    "Base exception for PGS parsing errors."
);
pyo3::create_exception!(
    pgs,
    PgsReadError,
    PgsError,
    "Error reading or parsing PGS segment data."
);
pyo3::create_exception!(
    pgs,
    PgsRleError,
    PgsError,
    "Error decoding RLE-compressed bitmap data."
);

pub fn to_py_err(e: pgs::Error) -> PyErr {
    match e {
        pgs::Error::Io(io_err) => PyIOError::new_err(io_err.to_string()),
        pgs::Error::Read { context, source } => {
            PgsReadError::new_err(format!("{context}: {source}"))
        }
        pgs::Error::Rle(rle_err) => PgsRleError::new_err(rle_err.to_string()),
        _ => PgsError::new_err(e.to_string()),
    }
}

/// Raise ValueError for a consumed reader.
pub fn consumed_err() -> PyErr {
    PyValueError::new_err("reader has already been consumed")
}

pub fn register(parent: &Bound<'_, PyModule>) -> PyResult<()> {
    parent.add("PgsError", parent.py().get_type::<PgsError>())?;
    parent.add("PgsReadError", parent.py().get_type::<PgsReadError>())?;
    parent.add("PgsRleError", parent.py().get_type::<PgsRleError>())?;
    Ok(())
}
