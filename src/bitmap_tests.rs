use super::*;

#[skuld::test]
fn new_and_dimensions() {
    let bmp = Bitmap::new(4, 3, vec![0u8; 12]);
    assert_eq!(bmp.width(), 4);
    assert_eq!(bmp.height(), 3);
    assert_eq!(bmp.data().len(), 12);
}

#[skuld::test]
fn new_wrong_size_panics() {
    let result = std::panic::catch_unwind(|| Bitmap::new(4, 3, vec![0u8; 10]));
    let msg = result.unwrap_err().downcast::<String>().unwrap();
    assert!(
        msg.contains("data length"),
        "unexpected panic message: {msg}"
    );
}

#[skuld::test]
fn row_access() {
    let bmp = Bitmap::new(3, 2, vec![1, 2, 3, 4, 5, 6]);
    assert_eq!(bmp.row(0), &[1, 2, 3]);
    assert_eq!(bmp.row(1), &[4, 5, 6]);
}

#[skuld::test]
fn get_pixel() {
    let bmp = Bitmap::new(3, 2, vec![10, 20, 30, 40, 50, 60]);
    assert_eq!(*bmp.get(0, 0), 10);
    assert_eq!(*bmp.get(2, 0), 30);
    assert_eq!(*bmp.get(1, 1), 50);
}

#[skuld::test]
fn get_mut_pixel() {
    let mut bmp = Bitmap::new(2, 2, vec![1, 2, 3, 4]);
    *bmp.get_mut(1, 0) = 99;
    assert_eq!(*bmp.get(1, 0), 99);
}

#[skuld::test]
fn map_bitmap() {
    let bmp = Bitmap::new(2, 2, vec![1u8, 2, 3, 4]);
    let doubled = bmp.map(|&v| v as u16 * 2);
    assert_eq!(doubled.data(), &[2u16, 4, 6, 8]);
    assert_eq!(doubled.width(), 2);
    assert_eq!(doubled.height(), 2);
}

#[skuld::test]
fn new_default() {
    let bmp: Bitmap<u8> = Bitmap::new_default(3, 2);
    assert_eq!(bmp.data(), &[0, 0, 0, 0, 0, 0]);
}

#[skuld::test]
fn into_data() {
    let data = vec![1u8, 2, 3, 4];
    let bmp = Bitmap::new(2, 2, data.clone());
    assert_eq!(bmp.into_data(), data);
}
