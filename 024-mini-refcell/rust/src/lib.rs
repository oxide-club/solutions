// SPDX-License-Identifier: MIT
//! Borrows dinâmicos com permissões conservadas até o drop de cada guard.

use std::cell::{Cell, UnsafeCell};
use std::ops::{Deref, DerefMut};

const FREE: i16 = 0;
const WRITER: i16 = -1;
const MAX_READERS: i16 = 255;

#[derive(Debug, PartialEq, Eq)]
pub enum BorrowError {
    WriterActive,
    ReadersActive,
    ReaderLimitReached,
}

pub struct MiniRefCell<T> {
    value: UnsafeCell<T>,
    state: Cell<i16>,
}

/// A referência à célula impede Send/Sync, pois MiniRefCell é !Sync.
pub struct MiniRef<'a, T> {
    cell: &'a MiniRefCell<T>,
}

pub struct MiniRefMut<'a, T> {
    cell: &'a MiniRefCell<T>,
}

impl<T> MiniRefCell<T> {
    pub fn new(value: T) -> Self {
        Self {
            value: UnsafeCell::new(value),
            state: Cell::new(FREE),
        }
    }

    pub fn try_borrow(&self) -> Result<MiniRef<'_, T>, BorrowError> {
        let readers = self.state.get();
        if readers == WRITER {
            return Err(BorrowError::WriterActive);
        }
        if readers == MAX_READERS {
            return Err(BorrowError::ReaderLimitReached);
        }
        // Somar somente abaixo do limite impede overflow para o estado livre.
        self.state.set(readers + 1);
        Ok(MiniRef { cell: self })
    }

    pub fn try_borrow_mut(&self) -> Result<MiniRefMut<'_, T>, BorrowError> {
        match self.state.get() {
            WRITER => Err(BorrowError::WriterActive),
            FREE => {
                self.state.set(WRITER);
                Ok(MiniRefMut { cell: self })
            }
            _ => Err(BorrowError::ReadersActive),
        }
    }
}

impl<T> Deref for MiniRef<'_, T> {
    type Target = T;

    fn deref(&self) -> &T {
        // SAFETY: este guard mantém um leitor contado, impedindo escritores.
        // A referência vive somente durante o borrow do guard.
        unsafe { &*self.cell.value.get() }
    }
}

impl<T> Deref for MiniRefMut<'_, T> {
    type Target = T;

    fn deref(&self) -> &T {
        // SAFETY: o único escritor impede outros guards. O borrow de &self
        // também impede deref_mut simultâneo neste mesmo guard.
        unsafe { &*self.cell.value.get() }
    }
}

impl<T> DerefMut for MiniRefMut<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        // SAFETY: só existe um escritor, sem leitores, e &mut self impede
        // acessos simultâneos pelo mesmo guard enquanto a referência é usada.
        unsafe { &mut *self.cell.value.get() }
    }
}

impl<T> Drop for MiniRef<'_, T> {
    fn drop(&mut self) {
        // Um guard vivo possui uma unidade; o contador permanece em 0..255.
        self.cell.state.set(self.cell.state.get() - 1);
    }
}

impl<T> Drop for MiniRefMut<'_, T> {
    fn drop(&mut self) {
        self.cell.state.set(FREE);
    }
}
