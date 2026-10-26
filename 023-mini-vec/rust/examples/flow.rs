// SPDX-License-Identifier: MIT
use mini_vec::MiniVec;

fn main() {
    let mut values = MiniVec::new().unwrap();
    for value in [10_u16, 20, 30] {
        values.push(value).unwrap();
    }
    assert_eq!((values.len(), values.capacity()), (3, 4));
    assert_eq!(values.pop(), Some(30));
    assert_eq!(values.get(0), Some(&10));
}
