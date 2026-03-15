/// An owned 2D pixel buffer.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Bitmap<T> {
    data: Vec<T>,
    width: u16,
    height: u16,
}

/// Palette-indexed bitmap (raw RLE output).
pub type IndexedBitmap = Bitmap<u8>;

impl<T> Bitmap<T> {
    /// Create a bitmap from raw pixel data.
    ///
    /// # Panics
    /// Panics if `data.len() != width as usize * height as usize`.
    pub fn new(width: u16, height: u16, data: Vec<T>) -> Self {
        assert_eq!(
            data.len(),
            width as usize * height as usize,
            "data length {} does not match {}x{}",
            data.len(),
            width,
            height,
        );
        Self {
            data,
            width,
            height,
        }
    }

    pub fn width(&self) -> u16 {
        self.width
    }

    pub fn height(&self) -> u16 {
        self.height
    }

    pub fn data(&self) -> &[T] {
        &self.data
    }

    pub fn data_mut(&mut self) -> &mut [T] {
        &mut self.data
    }

    pub fn into_data(self) -> Vec<T> {
        self.data
    }

    pub fn row(&self, y: u16) -> &[T] {
        let start = y as usize * self.width as usize;
        &self.data[start..start + self.width as usize]
    }

    pub fn row_mut(&mut self, y: u16) -> &mut [T] {
        let start = y as usize * self.width as usize;
        let w = self.width as usize;
        &mut self.data[start..start + w]
    }

    pub fn get(&self, x: u16, y: u16) -> &T {
        debug_assert!(x < self.width && y < self.height);
        &self.data[y as usize * self.width as usize + x as usize]
    }

    pub fn get_mut(&mut self, x: u16, y: u16) -> &mut T {
        debug_assert!(x < self.width && y < self.height);
        let w = self.width as usize;
        &mut self.data[y as usize * w + x as usize]
    }

    /// Create a new bitmap by applying a function to each pixel.
    pub fn map<U>(&self, f: impl FnMut(&T) -> U) -> Bitmap<U> {
        Bitmap {
            data: self.data.iter().map(f).collect(),
            width: self.width,
            height: self.height,
        }
    }
}

impl<T: Default + Clone> Bitmap<T> {
    /// Create a bitmap filled with the default value.
    pub fn new_default(width: u16, height: u16) -> Self {
        Self {
            data: vec![T::default(); width as usize * height as usize],
            width,
            height,
        }
    }
}

#[cfg(test)]
#[path = "bitmap_tests.rs"]
mod tests;
