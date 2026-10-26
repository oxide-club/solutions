// SPDX-License-Identifier: MIT
//! Vetor de tamanho variável: o buffer contém valores somente em `0..len`.

pub mod support;

use std::{alloc::Layout, marker::PhantomData, mem, ptr::NonNull};
pub use support::Allocator;

#[derive(Debug, PartialEq, Eq)]
pub enum CreateError {
    UnsupportedZst,
}

#[derive(Debug, PartialEq, Eq)]
pub enum PushReason {
    CapacityOverflow,
    AllocFailed,
}

#[derive(Debug)]
pub struct PushError<T> {
    pub reason: PushReason,
    pub value: T,
}

pub struct MiniVec<T> {
    ptr: NonNull<T>,
    len: usize,
    cap: usize,
    allocator: Allocator,
    owner: PhantomData<T>,
}

/// Calcula o layout sem acessar o allocator; também verifica isize::MAX.
pub fn layout_for<T>(capacity: usize) -> Result<Layout, PushReason> {
    Layout::array::<T>(capacity).map_err(|_| PushReason::CapacityOverflow)
}

impl<T> MiniVec<T> {
    pub fn new() -> Result<Self, CreateError> {
        // O próprio adaptador usa Rc: rejeitamos ZST antes de criá-lo.
        if mem::size_of::<T>() == 0 {
            return Err(CreateError::UnsupportedZst);
        }
        Self::with_allocator(Allocator::default())
    }

    pub fn with_allocator(allocator: Allocator) -> Result<Self, CreateError> {
        if mem::size_of::<T>() == 0 {
            return Err(CreateError::UnsupportedZst);
        }
        Ok(Self {
            ptr: NonNull::dangling(),
            len: 0,
            cap: 0,
            allocator,
            owner: PhantomData,
        })
    }

    /// Cresce antes de alterar o vetor: falhas preservam ponteiro, len e cap.
    fn grow(&mut self) -> Result<(), PushReason> {
        let capacity = if self.cap == 0 {
            1
        } else {
            self.cap
                .checked_mul(2)
                .ok_or(PushReason::CapacityOverflow)?
        };
        let layout = layout_for::<T>(capacity)?;
        let allocation = self
            .allocator
            .allocate(layout)
            .ok_or(PushReason::AllocFailed)?;
        let next = allocation.cast::<T>();
        if self.cap != 0 {
            let previous_layout = layout_for::<T>(self.cap).expect("capacidade já validada");
            // SAFETY: são alocações distintas, alinhadas e válidas para len
            // valores T. A cópia transfere ownership; não destruímos a origem.
            unsafe {
                std::ptr::copy_nonoverlapping(self.ptr.as_ptr(), next.as_ptr(), self.len);
                self.allocator.deallocate(self.ptr.cast(), previous_layout);
            }
        }
        self.ptr = next;
        self.cap = capacity;
        Ok(())
    }

    pub fn push(&mut self, value: T) -> Result<(), PushError<T>> {
        if self.len == self.cap {
            if let Err(reason) = self.grow() {
                return Err(PushError { reason, value });
            }
        }
        // SAFETY: len < cap; o slot alinhado ainda não contém valor vivo.
        // &mut self impede referências simultâneas aos elementos do vetor.
        unsafe { self.ptr.as_ptr().add(self.len).write(value) };
        self.len += 1;
        Ok(())
    }

    pub fn pop(&mut self) -> Option<T> {
        if self.len == 0 {
            return None;
        }
        self.len -= 1;
        // SAFETY: o último slot era inicializado; retirar len transfere sua
        // responsabilidade ao chamador e impede novo drop pelo vetor.
        Some(unsafe { self.ptr.as_ptr().add(self.len).read() })
    }

    pub fn get(&self, index: usize) -> Option<&T> {
        if index >= self.len {
            return None;
        }
        // SAFETY: index aponta para valor inicializado. O lifetime é o de
        // &self, que impede pop, crescimento e drop enquanto usado.
        Some(unsafe { &*self.ptr.as_ptr().add(index) })
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn capacity(&self) -> usize {
        self.cap
    }
}

/// Libera só o armazenamento, inclusive se um destrutor de T panicar.
struct BufferRelease<'a> {
    ptr: NonNull<u8>,
    layout: Layout,
    allocator: &'a Allocator,
}

impl Drop for BufferRelease<'_> {
    fn drop(&mut self) {
        // SAFETY: o guard recebe a única alocação viva e seu layout original.
        // Ele é local ao drop e roda uma vez, mesmo durante unwind.
        unsafe { self.allocator.deallocate(self.ptr, self.layout) };
    }
}

impl<T> Drop for MiniVec<T> {
    fn drop(&mut self) {
        if self.cap == 0 {
            return;
        }
        let _release = BufferRelease {
            ptr: self.ptr.cast(),
            layout: layout_for::<T>(self.cap).expect("capacidade já validada"),
            allocator: &self.allocator,
        };
        while self.len != 0 {
            // Retiramos a posição antes do destrutor: panic não repete o drop.
            self.len -= 1;
            // SAFETY: a posição estava em 0..len e ainda contém um T vivo.
            // Se houver unwind, o guard libera o buffer sem reler seus slots.
            unsafe { std::ptr::drop_in_place(self.ptr.as_ptr().add(self.len)) };
        }
    }
}
