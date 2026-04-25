# BIR Call Lowering Exit From Memory Coordinator Runbook

Status: Active
Source Idea: ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md

## Purpose

Move `LirCallOp` lowering out of
`src/backend/bir/lir_to_bir/memory/coordinator.cpp` so the memory coordinator
stays focused on scalar and local-memory dispatch.

## Goal

`memory/coordinator.cpp` should dispatch calls through a small
`lower_call_inst(...)` member call while preserving all existing call ABI,
pointer provenance, and memory intrinsic behavior.

## Core Rule

This is an implementation-placement cleanup only. Do not change call semantics,
memory intrinsic semantics, pointer provenance behavior, or test expectations.

## Read First

- `ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`

## Current Targets

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`

Use a small new `.cpp` only if the existing file organization clearly requires
it. Do not add new `.hpp` files.

## Non-Goals

- Do not redesign call ABI lowering.
- Do not split all call lowering into multiple files.
- Do not move `memcpy` or `memset` implementation back into call lowering.
- Do not change memory load/store/GEP behavior.
- Do not redesign pointer-int casts, scalar binops, alloca, or GEP lowering.
- Do not introduce `MemoryLoweringState` or a call-lowering state owner.
- Do not rewrite test expectations.

## Working Model

- `memory/coordinator.cpp` owns instruction-family dispatch.
- `calling.cpp` owns `LirCallOp` lowering behavior.
- `memory/intrinsics.cpp` owns `memcpy`/`memset` runtime intrinsic
  specialization.
- `lowering.hpp` remains the complete private member index for
  `BirFunctionLowerer`.

The intended coordinator shape is:

```cpp
if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
  return lower_call_inst(*call, lowered_insts);
}
```

`lower_call_inst` must remain a `BirFunctionLowerer` member. Do not convert
call lowering into a free function that takes `BirFunctionLowerer& self`.

## Execution Rules

- Preserve behavior while moving code.
- Keep the coordinator diff minimal: replace the moved call branch with member
  dispatch.
- Preserve memory intrinsic integration by allowing `lower_call_inst` to keep
  calling `try_lower_direct_memory_intrinsic_call`.
- Keep moved helper usage and includes as close to the original behavior as
  possible.
- For every code-changing step, build before accepting the slice.
- Relevant proof must include direct calls, indirect calls, byval/sret calls,
  and `memcpy`/`memset` runtime intrinsic calls.

## Steps

### Step 1: Declare Call Lowering Member

Goal: make call lowering an explicit `BirFunctionLowerer` member target.

Primary target:

- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Inspect nearby private lowering member declarations.
- Add:

```cpp
bool lower_call_inst(const c4c::codegen::lir::LirCallOp& call,
                     std::vector<bir::Inst>* lowered_insts);
```

- Keep declaration placement consistent with existing BIR lowering members.

Completion check:

- `lowering.hpp` declares `lower_call_inst(...)` exactly once as a
  `BirFunctionLowerer` member.

### Step 2: Move The Existing Call Branch

Goal: move only the current `LirCallOp` lowering body from the memory
coordinator into call lowering implementation.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`

Actions:

- Locate the full `LirCallOp` branch in `memory/coordinator.cpp`.
- Move the direct and indirect call lowering body into
  `BirFunctionLowerer::lower_call_inst(...)`, preferably in `calling.cpp`.
- Preserve byval/sret argument lowering, call ABI metadata computation, callee
  resolution, and memory intrinsic detection behavior.
- Keep `try_lower_direct_memory_intrinsic_call` integration intact.
- Move only includes/helpers required by the moved body.

Completion check:

- `memory/coordinator.cpp` no longer contains the full `LirCallOp` lowering
  body.
- `calling.cpp` or the selected implementation file defines
  `BirFunctionLowerer::lower_call_inst(...)`.
- No call ABI or memory intrinsic logic is rewritten while moving it.

### Step 3: Reduce Coordinator To Dispatch

Goal: leave the memory coordinator responsible only for call-family dispatch.

Primary target:

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`

Actions:

- Replace the moved call branch with:

```cpp
if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
  return lower_call_inst(*call, lowered_insts);
}
```

- Remove includes or local helpers that are only needed by the moved call body.
- Do not touch unrelated instruction branches.

Completion check:

- The coordinator call branch is a small dispatch to `lower_call_inst(...)`.
- No unrelated memory, scalar, alloca, GEP, load/store, or pointer-cast code is
  changed.

### Step 4: Build And Narrow Proof

Goal: prove the placement cleanup did not change behavior.

Actions:

- Build `c4c_codegen`.
- Run the relevant BIR/LIR-to-BIR tests that cover direct calls, indirect
  calls, byval/sret calls, and `memcpy`/`memset` runtime intrinsic calls.
- Do not rewrite expectations.

Completion check:

- `c4c_codegen` builds.
- The selected call and memory intrinsic tests pass.
- Any failure is investigated as a behavior-preservation problem, not masked by
  expectation changes.

### Step 5: Final Lifecycle Readiness Check

Goal: make the completed slice easy for the supervisor to validate and commit.

Actions:

- Confirm `memory/coordinator.cpp` only dispatches call lowering.
- Confirm call lowering remains a `BirFunctionLowerer` member.
- Confirm `memory/intrinsics.cpp` remains the owner of `memcpy`/`memset`
  specialization.
- Confirm no new `.hpp` files were added.
- Confirm no test expectations changed.

Completion check:

- All source acceptance criteria from the idea are satisfied, or remaining
  issues are recorded in `todo.md` for supervisor routing.
