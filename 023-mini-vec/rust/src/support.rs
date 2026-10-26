// SPDX-License-Identifier: MIT
use std::{
    alloc::{alloc, dealloc, Layout},
    cell::Cell,
    ptr::NonNull,
    rc::Rc,
};
#[derive(Clone, Default, Debug)]
pub struct Allocator(Rc<State>);
#[derive(Default, Debug)]
struct State {
    fail: Cell<bool>,
    attempts: Cell<usize>,
    allocations: Cell<usize>,
    frees: Cell<usize>,
}
impl Allocator {
    pub fn fail_next(&self) {
        self.0.fail.set(true)
    }
    pub fn stats(&self) -> (usize, usize, usize) {
        (
            self.0.attempts.get(),
            self.0.allocations.get(),
            self.0.frees.get(),
        )
    }
    /// Allocates a nonzero layout. Null refusal preserves all earlier allocations.
    pub fn allocate(&self, layout: Layout) -> Option<NonNull<u8>> {
        assert!(layout.size() > 0);
        self.0.attempts.set(self.0.attempts.get() + 1);
        if self.0.fail.replace(false) {
            return None;
        }
        // SAFETY: caller supplies a valid nonzero Layout; ownership is returned.
        let p = NonNull::new(unsafe { alloc(layout) })?;
        self.0.allocations.set(self.0.allocations.get() + 1);
        Some(p)
    }
    /// # Safety
    /// `p` must be a live allocation returned here with this exact layout, freed once.
    pub unsafe fn deallocate(&self, p: NonNull<u8>, layout: Layout) {
        // SAFETY: delegated precondition above.
        unsafe { dealloc(p.as_ptr(), layout) };
        self.0.frees.set(self.0.frees.get() + 1);
    }
}
