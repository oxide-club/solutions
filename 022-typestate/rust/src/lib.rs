// SPDX-License-Identifier: MIT
//! Buffer com operações disponíveis conforme o estado do recurso.
#![forbid(unsafe_code)]

pub mod support;

use std::marker::PhantomData;
pub use support::FaultPlan;

const MAX_BYTES: usize = 4096;

#[derive(Debug)]
pub struct Closed;
#[derive(Debug)]
pub struct Open;
#[derive(Debug)]
pub struct Finalized;

/// Os campos privados impedem construir um recurso em estado intermediário.
#[derive(Debug)]
pub struct Resource<S> {
    data: Vec<u8>,
    plan: FaultPlan,
    state: PhantomData<S>,
}

#[derive(Debug, PartialEq, Eq)]
pub enum TransitionReason {
    Injected,
}

/// Devolve ownership no estado de origem para permitir nova tentativa.
#[derive(Debug)]
pub struct TransitionError<S> {
    pub reason: TransitionReason,
    pub resource: Resource<S>,
}

#[derive(Debug, PartialEq, Eq)]
pub enum WriteError {
    CapacityExceeded,
    Injected,
}

impl Resource<Closed> {
    pub fn new(plan: FaultPlan) -> Self {
        Self {
            data: Vec::new(),
            plan,
            state: PhantomData,
        }
    }

    pub fn open(mut self) -> Result<Resource<Open>, TransitionError<Closed>> {
        if self.plan.take_open() {
            return Err(TransitionError {
                reason: TransitionReason::Injected,
                resource: self,
            });
        }
        Ok(Resource {
            data: self.data,
            plan: self.plan,
            state: PhantomData,
        })
    }
}

impl Resource<Open> {
    pub fn write(&mut self, bytes: &[u8]) -> Result<(), WriteError> {
        // A subtração é válida pelo invariante data.len() <= MAX_BYTES.
        // Verificar antes da soma evita overflow e preserva a falha pendente.
        if bytes.len() > MAX_BYTES - self.data.len() {
            return Err(WriteError::CapacityExceeded);
        }
        if bytes.is_empty() {
            return Ok(());
        }
        if self.plan.take_write() {
            return Err(WriteError::Injected);
        }
        self.data.extend_from_slice(bytes);
        Ok(())
    }

    pub fn finalize(mut self) -> Result<Resource<Finalized>, TransitionError<Open>> {
        if self.plan.take_finalize() {
            return Err(TransitionError {
                reason: TransitionReason::Injected,
                resource: self,
            });
        }
        Ok(Resource {
            data: self.data,
            plan: self.plan,
            state: PhantomData,
        })
    }
}

impl Resource<Finalized> {
    pub fn bytes(&self) -> &[u8] {
        &self.data
    }
}
