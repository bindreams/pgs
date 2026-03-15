/// Native PGS color: Y'CbCr BT.709 with alpha.
///
/// Field order matches the PDS wire format: Y, Cr, Cb, Alpha.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct YCbCrA {
    pub y: u8,
    pub cr: u8,
    pub cb: u8,
    pub alpha: u8,
}

/// Standard RGBA color.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Rgba {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl From<YCbCrA> for Rgba {
    fn from(c: YCbCrA) -> Self {
        // BT.709 Y'CbCr to RGB conversion
        // Y  is in [16, 235], Cb/Cr in [16, 240]
        let y = c.y as f64 - 16.0;
        let cb = c.cb as f64 - 128.0;
        let cr = c.cr as f64 - 128.0;

        let r = 1.164383 * y + 1.792741 * cr;
        let g = 1.164383 * y - 0.213249 * cb - 0.532909 * cr;
        let b = 1.164383 * y + 2.112402 * cb;

        Rgba {
            r: r.round().clamp(0.0, 255.0) as u8,
            g: g.round().clamp(0.0, 255.0) as u8,
            b: b.round().clamp(0.0, 255.0) as u8,
            a: c.alpha,
        }
    }
}

impl From<Rgba> for YCbCrA {
    fn from(c: Rgba) -> Self {
        // BT.709 RGB to Y'CbCr conversion
        let r = c.r as f64;
        let g = c.g as f64;
        let b = c.b as f64;

        let y = 16.0 + 0.182586 * r + 0.614231 * g + 0.062007 * b;
        let cb = 128.0 - 0.100644 * r - 0.338572 * g + 0.439216 * b;
        let cr = 128.0 + 0.439216 * r - 0.398942 * g - 0.040274 * b;

        YCbCrA {
            y: y.round().clamp(16.0, 235.0) as u8,
            cr: cr.round().clamp(16.0, 240.0) as u8,
            cb: cb.round().clamp(16.0, 240.0) as u8,
            alpha: c.a,
        }
    }
}

#[cfg(test)]
#[path = "color_tests.rs"]
mod tests;
