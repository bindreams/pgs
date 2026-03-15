use pyo3::prelude::*;

use crate::color::PyYCbCrA;
use crate::time::PyTimestamp;

// CompositionState ====================================================================================================

#[pyclass(name = "CompositionState", module = "pgs", frozen, eq, eq_int)]
#[derive(Clone, PartialEq)]
pub enum PyCompositionState {
    Normal = 0,
    AcquisitionPoint = 1,
    EpochStart = 2,
}

impl From<pgs::segment::CompositionState> for PyCompositionState {
    fn from(s: pgs::segment::CompositionState) -> Self {
        match s {
            pgs::segment::CompositionState::Normal => Self::Normal,
            pgs::segment::CompositionState::AcquisitionPoint => Self::AcquisitionPoint,
            pgs::segment::CompositionState::EpochStart => Self::EpochStart,
        }
    }
}

// CropRect ============================================================================================================

#[pyclass(name = "CropRect", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyCropRect {
    inner: pgs::segment::CropRect,
}

#[pymethods]
impl PyCropRect {
    #[getter]
    fn x(&self) -> u16 {
        self.inner.x
    }

    #[getter]
    fn y(&self) -> u16 {
        self.inner.y
    }

    #[getter]
    fn width(&self) -> u16 {
        self.inner.width
    }

    #[getter]
    fn height(&self) -> u16 {
        self.inner.height
    }

    fn __repr__(&self) -> String {
        format!(
            "CropRect(x={}, y={}, width={}, height={})",
            self.inner.x, self.inner.y, self.inner.width, self.inner.height
        )
    }
}

// CompositionObject ===================================================================================================

#[pyclass(name = "CompositionObject", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyCompositionObject {
    inner: pgs::segment::CompositionObject,
}

#[pymethods]
impl PyCompositionObject {
    #[getter]
    fn object_id(&self) -> u16 {
        self.inner.object_id
    }

    #[getter]
    fn window_id(&self) -> u8 {
        self.inner.window_id
    }

    #[getter]
    fn x(&self) -> u16 {
        self.inner.x
    }

    #[getter]
    fn y(&self) -> u16 {
        self.inner.y
    }

    #[getter]
    fn crop(&self) -> Option<PyCropRect> {
        self.inner.crop.map(|c| PyCropRect { inner: c })
    }

    fn __repr__(&self) -> String {
        format!(
            "CompositionObject(object_id={}, window_id={}, x={}, y={})",
            self.inner.object_id, self.inner.window_id, self.inner.x, self.inner.y
        )
    }
}

impl From<pgs::segment::CompositionObject> for PyCompositionObject {
    fn from(co: pgs::segment::CompositionObject) -> Self {
        Self { inner: co }
    }
}

// Window ==============================================================================================================

#[pyclass(name = "Window", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyWindow {
    inner: pgs::segment::Window,
}

#[pymethods]
impl PyWindow {
    #[getter]
    fn id(&self) -> u8 {
        self.inner.id
    }

    #[getter]
    fn x(&self) -> u16 {
        self.inner.x
    }

    #[getter]
    fn y(&self) -> u16 {
        self.inner.y
    }

    #[getter]
    fn width(&self) -> u16 {
        self.inner.width
    }

    #[getter]
    fn height(&self) -> u16 {
        self.inner.height
    }

    fn __repr__(&self) -> String {
        format!(
            "Window(id={}, x={}, y={}, width={}, height={})",
            self.inner.id, self.inner.x, self.inner.y, self.inner.width, self.inner.height
        )
    }
}

impl From<pgs::segment::Window> for PyWindow {
    fn from(w: pgs::segment::Window) -> Self {
        Self { inner: w }
    }
}

// PaletteEntry ========================================================================================================

#[pyclass(name = "PaletteEntry", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyPaletteEntry {
    inner: pgs::segment::PaletteEntry,
}

#[pymethods]
impl PyPaletteEntry {
    #[getter]
    fn id(&self) -> u8 {
        self.inner.id
    }

    #[getter]
    fn color(&self) -> PyYCbCrA {
        PyYCbCrA::from(self.inner.color)
    }

    fn __repr__(&self) -> String {
        format!("PaletteEntry(id={})", self.inner.id)
    }
}

impl From<pgs::segment::PaletteEntry> for PyPaletteEntry {
    fn from(pe: pgs::segment::PaletteEntry) -> Self {
        Self { inner: pe }
    }
}

// PaletteDefinition ===================================================================================================

#[pyclass(name = "PaletteDefinition", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyPaletteDefinition {
    inner: pgs::segment::PaletteDefinition,
}

#[pymethods]
impl PyPaletteDefinition {
    #[getter]
    fn id(&self) -> u8 {
        self.inner.id
    }

    #[getter]
    fn version(&self) -> u8 {
        self.inner.version
    }

    #[getter]
    fn entries(&self) -> Vec<PyPaletteEntry> {
        self.inner
            .entries
            .iter()
            .map(|e| PyPaletteEntry::from(*e))
            .collect()
    }

    fn __repr__(&self) -> String {
        format!(
            "PaletteDefinition(id={}, version={}, entries={})",
            self.inner.id,
            self.inner.version,
            self.inner.entries.len()
        )
    }
}

impl From<pgs::segment::PaletteDefinition> for PyPaletteDefinition {
    fn from(pd: pgs::segment::PaletteDefinition) -> Self {
        Self { inner: pd }
    }
}

// PresentationComposition =============================================================================================

#[pyclass(name = "PresentationComposition", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyPresentationComposition {
    inner: pgs::segment::PresentationComposition,
}

#[pymethods]
impl PyPresentationComposition {
    #[getter]
    fn width(&self) -> u16 {
        self.inner.width
    }

    #[getter]
    fn height(&self) -> u16 {
        self.inner.height
    }

    #[getter]
    fn framerate(&self) -> u8 {
        self.inner.framerate
    }

    #[getter]
    fn composition_number(&self) -> u16 {
        self.inner.composition_number
    }

    #[getter]
    fn state(&self) -> PyCompositionState {
        self.inner.state.into()
    }

    #[getter]
    fn palette_update_id(&self) -> Option<u8> {
        self.inner.palette_update_id
    }

    #[getter]
    fn composition_objects(&self) -> Vec<PyCompositionObject> {
        self.inner
            .composition_objects
            .iter()
            .map(|co| PyCompositionObject::from(co.clone()))
            .collect()
    }

    fn __repr__(&self) -> String {
        format!(
            "PresentationComposition({}x{}, state={:?}, objects={})",
            self.inner.width,
            self.inner.height,
            self.inner.state,
            self.inner.composition_objects.len()
        )
    }
}

// WindowDefinition ====================================================================================================

#[pyclass(name = "WindowDefinition", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyWindowDefinition {
    inner: pgs::segment::WindowDefinition,
}

#[pymethods]
impl PyWindowDefinition {
    #[getter]
    fn windows(&self) -> Vec<PyWindow> {
        self.inner
            .windows
            .iter()
            .map(|w| PyWindow::from(*w))
            .collect()
    }

    fn __repr__(&self) -> String {
        format!("WindowDefinition(windows={})", self.inner.windows.len())
    }
}

// ObjectDefinition ====================================================================================================

#[pyclass(name = "ObjectDefinition", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PyObjectDefinition {
    inner: pgs::segment::ObjectDefinition,
}

#[pymethods]
impl PyObjectDefinition {
    #[getter]
    fn id(&self) -> u16 {
        self.inner.id
    }

    #[getter]
    fn version(&self) -> u8 {
        self.inner.version
    }

    #[getter]
    fn is_first(&self) -> bool {
        self.inner
            .sequence
            .contains(pgs::segment::SequenceFlag::FIRST)
    }

    #[getter]
    fn is_last(&self) -> bool {
        self.inner
            .sequence
            .contains(pgs::segment::SequenceFlag::LAST)
    }

    #[getter]
    fn data<'py>(&self, py: Python<'py>) -> Bound<'py, pyo3::types::PyBytes> {
        pyo3::types::PyBytes::new(py, &self.inner.data)
    }

    fn __repr__(&self) -> String {
        format!(
            "ObjectDefinition(id={}, version={}, data_len={})",
            self.inner.id,
            self.inner.version,
            self.inner.data.len()
        )
    }
}

// SegmentBody =========================================================================================================

#[pyclass(name = "SegmentBody", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PySegmentBody {
    inner: pgs::SegmentBody,
}

#[pymethods]
impl PySegmentBody {
    #[getter]
    fn kind(&self) -> &str {
        match &self.inner {
            pgs::SegmentBody::PresentationComposition(_) => "presentation_composition",
            pgs::SegmentBody::WindowDefinition(_) => "window_definition",
            pgs::SegmentBody::PaletteDefinition(_) => "palette_definition",
            pgs::SegmentBody::ObjectDefinition(_) => "object_definition",
            pgs::SegmentBody::End => "end",
        }
    }

    fn as_presentation_composition(&self) -> Option<PyPresentationComposition> {
        match &self.inner {
            pgs::SegmentBody::PresentationComposition(pcs) => {
                Some(PyPresentationComposition { inner: pcs.clone() })
            }
            _ => None,
        }
    }

    fn as_window_definition(&self) -> Option<PyWindowDefinition> {
        match &self.inner {
            pgs::SegmentBody::WindowDefinition(wds) => {
                Some(PyWindowDefinition { inner: wds.clone() })
            }
            _ => None,
        }
    }

    fn as_palette_definition(&self) -> Option<PyPaletteDefinition> {
        match &self.inner {
            pgs::SegmentBody::PaletteDefinition(pds) => {
                Some(PyPaletteDefinition { inner: pds.clone() })
            }
            _ => None,
        }
    }

    fn as_object_definition(&self) -> Option<PyObjectDefinition> {
        match &self.inner {
            pgs::SegmentBody::ObjectDefinition(ods) => {
                Some(PyObjectDefinition { inner: ods.clone() })
            }
            _ => None,
        }
    }

    fn __repr__(&self) -> String {
        format!("SegmentBody(kind='{}')", self.kind())
    }
}

impl From<pgs::SegmentBody> for PySegmentBody {
    fn from(sb: pgs::SegmentBody) -> Self {
        Self { inner: sb }
    }
}

// Segment =============================================================================================================

#[pyclass(name = "Segment", module = "pgs", frozen)]
#[derive(Clone)]
pub struct PySegment {
    inner: pgs::Segment,
}

#[pymethods]
impl PySegment {
    #[getter]
    fn pts(&self) -> PyTimestamp {
        self.inner.pts.into()
    }

    #[getter]
    fn dts(&self) -> PyTimestamp {
        self.inner.dts.into()
    }

    #[getter]
    fn body(&self) -> PySegmentBody {
        self.inner.body.clone().into()
    }

    fn is_end(&self) -> bool {
        self.inner.is_end()
    }

    fn composition_state(&self) -> Option<PyCompositionState> {
        self.inner.composition_state().map(Into::into)
    }

    fn __repr__(&self) -> String {
        let kind = match &self.inner.body {
            pgs::SegmentBody::PresentationComposition(_) => "PCS",
            pgs::SegmentBody::WindowDefinition(_) => "WDS",
            pgs::SegmentBody::PaletteDefinition(_) => "PDS",
            pgs::SegmentBody::ObjectDefinition(_) => "ODS",
            pgs::SegmentBody::End => "END",
        };
        format!("Segment({kind}, pts={})", self.inner.pts)
    }
}

impl From<pgs::Segment> for PySegment {
    fn from(s: pgs::Segment) -> Self {
        Self { inner: s }
    }
}
