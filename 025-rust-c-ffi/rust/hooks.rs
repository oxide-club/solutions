// SPDX-License-Identifier: MIT
// Suporte do clube. O gancho só existe na configuração test_hooks.
#[cfg(test_hooks)]
std::thread_local! { static ARMED: std::cell::Cell<bool> = const { std::cell::Cell::new(false) }; }
#[cfg(test_hooks)]
#[no_mangle]
pub extern "C" fn oc_test_panic_next() {
    ARMED.with(|armed| armed.set(true));
}
pub fn checkpoint() {
    #[cfg(test_hooks)]
    ARMED.with(|armed| {
        if armed.replace(false) {
            panic!("panic controlado de transformação");
        }
    });
}
