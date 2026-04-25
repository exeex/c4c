# Regalloc C++ vs Rust Comparison

This note continues Step 5 acceptance for the active prealloc migration by
summarizing the live C++ regalloc phase in `regalloc.cpp` against the
historical Rust design. The prealloc-local `regalloc.rs` file has been removed;
use `ref/claudes-c-compiler/` only for explicit archaeology.

## Active Match Points

### Three-lane allocation strategy and call-awareness

- Rust `allocate_registers(...)` allocates in three passes: call-spanning
  values first on callee-saved registers, non-call-spanning values next on
  caller-saved registers, then callee-saved spillover for the remaining
  non-call-crossing pressure.
- C++ `BirPreAlloc::run_regalloc()` now follows the same phase ordering through
  three `assign_from_pool(...)` passes over call-crossing values, caller-saved
  non-call values, and callee-saved spillover for the remaining non-call
  values.
- Both routes use liveness call-point information to keep call-crossing values
  out of caller-saved pools and to preserve the intended ABI-sensitive split
  between long-lived and short-lived values.

### Weighted priority, interval overlap, and eviction behavior

- Rust ranks allocation candidates by weighted use count and interval length so
  loop-hot values outrank colder straight-line values.
- C++ seeds each `PreparedRegallocValue` from prepared liveness, computes
  `priority` and `spill_weight` with loop-depth-weighted uses, and then sorts
  allocation order by live-interval start and descending priority.
- C++ goes beyond the Rust baseline by publishing explicit interference edges
  for overlapping live intervals and by allowing lower-ranked active
  assignments to be evicted when a stronger value arrives at the same register
  pool boundary.

### Stack fallback and prepared-contract publication

- Rust returns an assignment map plus the list of used physical registers,
  leaving stack residence implicit for values that do not fit.
- C++ projects the same allocation outcome into the active prepared contract:
  every `PreparedRegallocValue` becomes register-backed or stack-backed, home
  slot requirements are preserved, and new stack slots are allocated with the
  live stack-layout frame as the base.
- The active C++ route is runtime-proven by `backend_prepare_liveness`, which
  checks real register assignments, caller-vs-callee pool behavior, spillover
  into remaining callee-saved registers, home-slot preservation, and explicit
  spill/reload bookkeeping.

### Move resolution and ABI transfer metadata

- Rust focuses on value-to-register assignments and leaves downstream
  instruction-move planning to surrounding backend code.
- C++ adds explicit move-resolution bookkeeping for phi joins, consumer
  mismatches, call arguments, call results, and function returns. The active
  route records the source/destination storage kinds and concrete ABI register
  names where they are known.
- `backend_prepare_liveness` already proves these records matter under real
  prepared data by checking phi join moves, call-site argument/result
  transfers, return-site transfers, and redundant-move suppression when the
  assigned storage already matches the ABI destination.

## Bounded Divergences

### Intentional host-framework adaptations

- C++ allocates over prepared liveness records and writes a
  `PreparedRegallocFunction` contract instead of returning Rust's simpler
  `RegAllocResult` map of value IDs to physical registers.
- The C++ route also owns stack-slot materialization, interference publication,
  move-resolution records, and spill/reload operation metadata because those
  artifacts are first-class backend inputs in c4c's prepared pipeline.

### Register-pool scope is still deliberately smaller than Rust

- Rust is parameterized by `RegAllocConfig` and can be given larger register
  pools, including optional inline-asm participation where supported.
- The current C++ route uses a deliberately bounded built-in pool per target:
  one caller-saved general register and two callee-saved general registers,
  with no active float-register allocation path yet.
- This difference is visible rather than hidden: C++ still classifies float
  values, but the current pools only seed general-register allocation, so
  float values and any `PreparedRegisterClass::None` values fall back to stack
  slots or ABI move metadata instead of receiving direct physical-register
  assignments.

### Eligibility remains shaped by the prepared contract, not the raw IR API

- Rust filters raw IR values for eligibility with `remove_ineligible_operands`,
  non-GPR propagation, and config-driven inline-asm allowance.
- C++ inherits most eligibility from prepared liveness and prepared value
  kinds, then rejects values that require a permanent home slot or have no
  live interval / register class. That keeps the active c4c route honest, but
  it is not yet a one-to-one port of every Rust-side operand-eligibility rule.

## Runtime Proof

- Focused proof command:
  `cmake --build --preset default -j4 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_liveness$'`
- Current proof surface covers:
  register seeding from prepared liveness, call-crossing pool selection,
  caller-pool overflow into callee-saved spillover, weighted loop priority,
  home-slot preservation, spill/reload ops, phi join move resolution, and ABI
  move metadata for calls and returns.
