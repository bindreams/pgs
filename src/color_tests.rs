use super::*;

#[skuld::test]
fn ycbcra_to_rgba_black() {
    let c = YCbCrA {
        y: 16,
        cr: 128,
        cb: 128,
        alpha: 255,
    };
    let rgba: Rgba = c.into();
    assert_eq!(
        rgba,
        Rgba {
            r: 0,
            g: 0,
            b: 0,
            a: 255
        }
    );
}

#[skuld::test]
fn ycbcra_to_rgba_white() {
    let c = YCbCrA {
        y: 235,
        cr: 128,
        cb: 128,
        alpha: 255,
    };
    let rgba: Rgba = c.into();
    assert_eq!(
        rgba,
        Rgba {
            r: 255,
            g: 255,
            b: 255,
            a: 255
        }
    );
}

#[skuld::test]
fn ycbcra_to_rgba_preserves_alpha() {
    let c = YCbCrA {
        y: 16,
        cr: 128,
        cb: 128,
        alpha: 128,
    };
    let rgba: Rgba = c.into();
    assert_eq!(rgba.a, 128);
}

#[skuld::test]
fn rgba_to_ycbcra_black() {
    let c = Rgba {
        r: 0,
        g: 0,
        b: 0,
        a: 255,
    };
    let ycbcr: YCbCrA = c.into();
    assert_eq!(
        ycbcr,
        YCbCrA {
            y: 16,
            cr: 128,
            cb: 128,
            alpha: 255
        }
    );
}

#[skuld::test]
fn rgba_to_ycbcra_white() {
    let c = Rgba {
        r: 255,
        g: 255,
        b: 255,
        a: 255,
    };
    let ycbcr: YCbCrA = c.into();
    assert_eq!(
        ycbcr,
        YCbCrA {
            y: 235,
            cr: 128,
            cb: 128,
            alpha: 255
        }
    );
}

#[skuld::test]
fn rgba_to_ycbcra_preserves_alpha() {
    let c = Rgba {
        r: 0,
        g: 0,
        b: 0,
        a: 42,
    };
    let ycbcr: YCbCrA = c.into();
    assert_eq!(ycbcr.alpha, 42);
}

/// Round-trip through RGB→YCbCr→RGB should be close to identity.
#[skuld::test]
fn roundtrip_red() {
    let red = Rgba {
        r: 255,
        g: 0,
        b: 0,
        a: 255,
    };
    let ycbcr: YCbCrA = red.into();
    let back: Rgba = ycbcr.into();
    assert!((back.r as i16 - 255).abs() <= 1, "r={}", back.r);
    assert!((back.g as i16).abs() <= 1, "g={}", back.g);
    assert!((back.b as i16).abs() <= 1, "b={}", back.b);
}

/// Round-trip through RGB→YCbCr→RGB for green.
#[skuld::test]
fn roundtrip_green() {
    let green = Rgba {
        r: 0,
        g: 255,
        b: 0,
        a: 255,
    };
    let ycbcr: YCbCrA = green.into();
    let back: Rgba = ycbcr.into();
    assert!((back.r as i16).abs() <= 1, "r={}", back.r);
    assert!((back.g as i16 - 255).abs() <= 1, "g={}", back.g);
    assert!((back.b as i16).abs() <= 1, "b={}", back.b);
}

/// Round-trip through RGB→YCbCr→RGB for blue.
#[skuld::test]
fn roundtrip_blue() {
    let blue = Rgba {
        r: 0,
        g: 0,
        b: 255,
        a: 255,
    };
    let ycbcr: YCbCrA = blue.into();
    let back: Rgba = ycbcr.into();
    assert!((back.r as i16).abs() <= 1, "r={}", back.r);
    assert!((back.g as i16).abs() <= 1, "g={}", back.g);
    assert!((back.b as i16 - 255).abs() <= 1, "b={}", back.b);
}
