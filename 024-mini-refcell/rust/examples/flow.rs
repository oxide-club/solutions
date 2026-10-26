// SPDX-License-Identifier: MIT
use mini_refcell::{BorrowError, MiniRefCell};

fn main() {
    let cell = MiniRefCell::new(7);
    let reader = cell.try_borrow().unwrap();
    assert_eq!(
        cell.try_borrow_mut().err(),
        Some(BorrowError::ReadersActive)
    );
    assert_eq!(*reader, 7);
    drop(reader);
    *cell.try_borrow_mut().unwrap() = 9;
    assert_eq!(*cell.try_borrow().unwrap(), 9);
}
