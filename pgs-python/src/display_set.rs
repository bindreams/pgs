use std::collections::HashMap;

use pyo3::prelude::*;

use crate::bitmap::PyBitmap;
use crate::segment::{PyCompositionObject, PyCompositionState, PyPaletteDefinition, PyWindow};
use crate::time::{PyDuration, PyTimestamp};

#[pyclass(name = "DisplaySet", module = "pgs", frozen)]
pub struct PyDisplaySet {
    pub timestamp: pgs::time::Timestamp,
    pub duration: pgs::time::Duration,
    pub state: pgs::segment::CompositionState,
    pub windows: HashMap<u8, pgs::segment::Window>,
    pub palettes: HashMap<u8, pgs::segment::PaletteDefinition>,
    pub objects: HashMap<u16, pgs::bitmap::Bitmap<u8>>,
    pub compositions: Vec<pgs::segment::CompositionObject>,
    pub active_palette_id: u8,
}

impl PyDisplaySet {
    pub fn from_display_set(ds: pgs::DisplaySet) -> Self {
        Self {
            timestamp: ds.timestamp,
            duration: ds.duration,
            state: ds.state,
            windows: ds.windows,
            palettes: ds.palettes,
            objects: ds.objects,
            compositions: ds.compositions,
            active_palette_id: ds.active_palette_id,
        }
    }
}

#[pymethods]
impl PyDisplaySet {
    #[getter]
    fn timestamp(&self) -> PyTimestamp {
        self.timestamp.into()
    }

    #[getter]
    fn duration(&self) -> PyDuration {
        self.duration.into()
    }

    #[getter]
    fn state(&self) -> PyCompositionState {
        self.state.into()
    }

    #[getter]
    fn active_palette_id(&self) -> u8 {
        self.active_palette_id
    }

    #[getter]
    fn windows(&self) -> HashMap<u8, PyWindow> {
        self.windows
            .iter()
            .map(|(&k, v)| (k, PyWindow::from(*v)))
            .collect()
    }

    #[getter]
    fn palettes(&self) -> HashMap<u8, PyPaletteDefinition> {
        self.palettes
            .iter()
            .map(|(&k, v)| (k, PyPaletteDefinition::from(v.clone())))
            .collect()
    }

    #[getter]
    fn objects(&self) -> HashMap<u16, PyBitmap> {
        self.objects
            .iter()
            .map(|(&k, v)| (k, PyBitmap::from_indexed_bitmap(v)))
            .collect()
    }

    #[getter]
    fn compositions(&self) -> Vec<PyCompositionObject> {
        self.compositions
            .iter()
            .map(|co| PyCompositionObject::from(co.clone()))
            .collect()
    }

    fn is_empty(&self) -> bool {
        self.compositions.is_empty()
    }

    fn __repr__(&self) -> String {
        format!(
            "DisplaySet(timestamp={}, state={:?}, compositions={})",
            self.timestamp,
            self.state,
            self.compositions.len()
        )
    }
}
