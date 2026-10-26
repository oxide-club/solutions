// SPDX-License-Identifier: MIT
use typestate::{FaultPlan, Resource};

fn main() {
    let mut resource = Resource::new(FaultPlan {
        finalize: true,
        ..FaultPlan::default()
    })
    .open()
    .unwrap();
    resource.write(b"abcd").unwrap();
    // O erro conserva ownership e permite tentar finalizar novamente.
    let error = resource.finalize().unwrap_err();
    let finalized = error.resource.finalize().unwrap();
    assert_eq!(finalized.bytes(), b"abcd");
}
