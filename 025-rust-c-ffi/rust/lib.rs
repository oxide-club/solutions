// SPDX-License-Identifier: MIT
//! ABI C para uma transformação didática de bytes com contexto Rust opaco.

mod hooks;

use std::panic::{catch_unwind, AssertUnwindSafe};

const OK: i32 = 0;
const INVALID_ARGUMENT: i32 = 1;
const BUFFER_TOO_SMALL: i32 = 2;
const INTERNAL_PANIC: i32 = 3;
const POISONED: i32 = 4;

#[repr(C)]
pub struct OcResult {
    pub status: i32,
    pub written: usize,
    pub required: usize,
}

pub struct OcContext {
    key: u8,
    poisoned: bool,
}

extern "C" {
    /// O helper C aceita qualquer byte e não faz unwind nem longjmp.
    pub fn oc_rotate(value: u8) -> u8;
}

fn error(status: i32) -> OcResult {
    OcResult {
        status,
        written: 0,
        required: 0,
    }
}

/// Um payload arbitrário pode ter destrutor que panica. Capturamos esse
/// segundo panic e retemos seu payload, para impedir unwind pela ABI C.
fn discard_panic(payload: Box<dyn std::any::Any + Send>) {
    if let Err(second) = catch_unwind(AssertUnwindSafe(|| drop(payload))) {
        std::mem::forget(second);
    }
}

/// Cria um contexto e transfere seu ownership ao cliente.
///
/// # Safety
/// Se não for nulo, `out` aponta para um slot alinhado, vivo e gravável de
/// `*mut OcContext`. O cliente deve destruir o handle publicado uma única vez.
#[no_mangle]
pub unsafe extern "C" fn oc_create(key: u8, out: *mut *mut OcContext) -> i32 {
    if out.is_null() {
        return INVALID_ARGUMENT;
    }
    // SAFETY: o slot válido e exclusivo é uma pré-condição do chamador.
    unsafe { out.write(std::ptr::null_mut()) };
    match catch_unwind(|| {
        Box::new(OcContext {
            key,
            poisoned: false,
        })
    }) {
        Ok(context) => {
            // SAFETY: publicamos um handle vivo no mesmo slot validado acima.
            unsafe { out.write(Box::into_raw(context)) };
            OK
        }
        Err(payload) => {
            discard_panic(payload);
            INTERNAL_PANIC
        }
    }
}

/// Libera o contexto pelo mesmo allocator usado em sua criação.
///
/// # Safety
/// `ctx` é nulo ou um handle vivo retornado por oc_create, sem chamada
/// concorrente e sem uso posterior. O cliente transfere ownership de volta.
#[no_mangle]
pub unsafe extern "C" fn oc_destroy(ctx: *mut OcContext) {
    if !ctx.is_null() {
        // SAFETY: a pré-condição garante a origem e a transferência única.
        // Os campos u8/bool não executam destrutores que possam panicar.
        unsafe { drop(Box::from_raw(ctx)) };
    }
}

/// Transforma sem reter referências e sem deixar unwind atravessar a ABI.
///
/// # Safety
/// `ctx` não nulo deve estar vivo; chamadas sobre ele são serializadas.
/// Para comprimentos escalares válidos, src é legível/inicializado e dst é
/// gravável nas extensões declaradas. As regiões não vazias não se sobrepõem
/// entre si nem com ctx. Não há exigência de extensão para escalar rejeitado.
/// Ponteiros nulos só admitem seu respectivo comprimento igual a zero.
#[no_mangle]
pub unsafe extern "C" fn oc_transform(
    ctx: *mut OcContext,
    src: *const u8,
    src_len: usize,
    dst: *mut u8,
    dst_cap: usize,
) -> OcResult {
    // Rejeitamos escalares antes de consultar poison ou acessar os buffers.
    if ctx.is_null()
        || (src.is_null() && src_len != 0)
        || (dst.is_null() && dst_cap != 0)
        || src_len > isize::MAX as usize
        || dst_cap > isize::MAX as usize
    {
        return error(INVALID_ARGUMENT);
    }
    // SAFETY: ctx não nulo é um handle vivo e exclusivo por pré-condição.
    let context = unsafe { &mut *ctx };
    if context.poisoned {
        return error(POISONED);
    }
    if dst_cap < src_len {
        return OcResult {
            status: BUFFER_TOO_SMALL,
            written: 0,
            required: src_len,
        };
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        hooks::checkpoint();
        for index in 0..src_len {
            // SAFETY: index está nas duas regiões válidas e não sobrepostas.
            // O helper aceita qualquer byte e não lança exceções estrangeiras.
            unsafe {
                let byte = src.add(index).read();
                dst.add(index).write(oc_rotate(byte ^ context.key));
            }
        }
    }));
    match result {
        Ok(()) => OcResult {
            status: OK,
            written: src_len,
            required: src_len,
        },
        Err(payload) => {
            context.poisoned = true;
            discard_panic(payload);
            error(INTERNAL_PANIC)
        }
    }
}
