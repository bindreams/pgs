# Color Conversion

PGS palettes store colors in the **Y'CbCr** color space using the **BT.709** standard with **studio range** (also called "limited range")[^color-ref]. This library converts between Y'CbCr and standard RGBA.

## Y'CbCr color space

Y'CbCr separates a color into three components:

- **Y'** — luma (perceived brightness), a weighted sum of the red, green, and blue channels.
- **Cb** — blue-difference chroma. How far the color deviates from luma toward blue.
- **Cr** — red-difference chroma. How far the color deviates from luma toward red.

In studio range, the components are restricted to a subset of the full 8-bit range:

| Component | Range    | Neutral value |
| --------- | -------- | ------------- |
| Y'        | 16 – 235 | —             |
| Cb        | 16 – 240 | 128           |
| Cr        | 16 – 240 | 128           |

The alpha channel is passed through unchanged.

## Y'CbCr to RGB

To convert from Y'CbCr to RGB, first shift the components to center them around zero:

$$
y = Y - 16 \qquad c_b = C_b - 128 \qquad c_r = C_r - 128
$$

Then apply the BT.709 conversion matrix:

$$
\begin{bmatrix} R \\ G \\ B \end{bmatrix} =
\begin{bmatrix}
1.164383 & 0        & 1.792741  \\
1.164383 & -0.213249 & -0.532909 \\
1.164383 & 2.112402  & 0
\end{bmatrix}
\begin{bmatrix} y \\ c_b \\ c_r \end{bmatrix}
$$

Or equivalently, as individual equations:

$$
\begin{aligned}
R &= 1.164383 \cdot y + 1.792741 \cdot c_r \\
G &= 1.164383 \cdot y - 0.213249 \cdot c_b - 0.532909 \cdot c_r \\
B &= 1.164383 \cdot y + 2.112402 \cdot c_b
\end{aligned}
$$

Each result is rounded and clamped to $[0, 255]$.

## RGB to Y'CbCr

The inverse conversion takes an RGB triplet and produces studio-range Y'CbCr:

$$
\begin{bmatrix} Y \\ C_b \\ C_r \end{bmatrix} =
\begin{bmatrix} 16 \\ 128 \\ 128 \end{bmatrix} +
\begin{bmatrix}
 0.182586 &  0.614231 &  0.062007 \\
-0.100644 & -0.338572 &  0.439216 \\
 0.439216 & -0.398942 & -0.040274
\end{bmatrix}
\begin{bmatrix} R \\ G \\ B \end{bmatrix}
$$

Results are rounded and clamped: $Y \in [16, 235]$, $C_b, C_r \in [16, 240]$.

## Where the coefficients come from

The matrix entries are derived from the **BT.709 luma coefficients**:

$$
K_R = 0.2126 \qquad K_G = 0.7152 \qquad K_B = 0.0722
$$

These specify how much each primary contributes to perceived brightness. The full-range luma is:

$$
Y' = K_R \cdot R + K_G \cdot G + K_B \cdot B
$$

The chroma components are normalized blue- and red-difference signals:

$$
P_b = \frac{1}{2} \cdot \frac{B - Y'}{1 - K_B} \qquad P_r = \frac{1}{2} \cdot \frac{R - Y'}{1 - K_R}
$$

To map these into 8-bit studio range, the signals are scaled:

- **Y**: mapped from $[0, 1]$ to $[16, 235]$ — a scale factor of $219$, so $Y = 16 + 219 \cdot Y'$
- **Cb, Cr**: mapped from $[-0.5, 0.5]$ to $[16, 240]$ — a scale factor of $224$, so $C_b = 128 + 224 \cdot P_b$

The decoding factor $1.164383 \approx 255 / 219$ reverses the Y scaling; the chroma factors follow similarly from $255 / 224$ and the luma coefficients.

## Implementation

The library performs the conversion in `f64` floating point, then rounds and clamps each component to `u8`. This matches the standard rounding behavior and is accurate to ±1 least significant bit on round-trip conversion.

[^color-ref]: [Equasys, "Color Conversion"](https://web.archive.org/web/20180421030430/http://www.equasys.de/colorconversion.html)
