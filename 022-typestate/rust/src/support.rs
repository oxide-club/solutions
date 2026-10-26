// SPDX-License-Identifier: MIT
/// Each true flag injects exactly one failure; operations consume independently.
#[derive(Debug, Default)]
pub struct FaultPlan {
    pub open: bool,
    pub write: bool,
    pub finalize: bool,
}
impl FaultPlan {
    pub fn take_open(&mut self) -> bool {
        std::mem::take(&mut self.open)
    }
    pub fn take_write(&mut self) -> bool {
        std::mem::take(&mut self.write)
    }
    pub fn take_finalize(&mut self) -> bool {
        std::mem::take(&mut self.finalize)
    }
}
