# BIR Call Lowering Exit From Memory Coordinator

## Intent

Move `LirCallOp` lowering out of
`src/backend/bir/lir_to_bir/memory/coordinator.cpp` so the memory coordinator
stays focused on scalar/local-memory dispatch.

This should be a narrow implementation-placement cleanup. It should not change
call ABI behavior, memory intrinsic behavior, pointer provenance behavior, or
test expectations.

## Rationale

After the memory intrinsic extraction, `memory/coordinator.cpp` still contains
the full direct/indirect call lowering branch. That branch parses typed calls,
handles byval/sret argument lowering, computes call ABI metadata, resolves
callee names, and invokes memory intrinsic lowering when the callee is
`memcpy`/`memset`.

Those responsibilities do not belong in the memory coordinator. The coordinator
should dispatch `LirCallOp` to a call-lowering member, while the call-lowering
implementation can still call memory intrinsic helpers when a direct runtime
memory intrinsic is detected.

## Design Direction

Keep ownership simple:

```text
memory/coordinator.cpp
  dispatches instruction families

calling.cpp
  owns LirCallOp lowering behavior

memory/intrinsics.cpp
  owns memcpy/memset runtime intrinsic specialization

lowering.hpp
  remains the complete private index
```

The desired coordinator shape is:

```cpp
if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
  return lower_call_inst(*call, lowered_insts);
}
```

`lower_call_inst` should remain a `BirFunctionLowerer` member. Do not convert
call lowering into a free function that takes `BirFunctionLowerer& self`.

## Scope

1. Add an explicit member declaration in `lowering.hpp`.
   - `bool lower_call_inst(const c4c::codegen::lir::LirCallOp& call,
                           std::vector<bir::Inst>* lowered_insts);`

2. Move only the current `LirCallOp` branch from `memory/coordinator.cpp`.
   - Move the direct/indirect call lowering body to the call-lowering
     implementation file.
   - Prefer `src/backend/bir/lir_to_bir/calling.cpp` unless the existing file
     organization clearly requires a small new `.cpp`.
   - Do not add new `.hpp` files.

3. Preserve memory intrinsic integration.
   - `lower_call_inst` may continue calling
     `try_lower_direct_memory_intrinsic_call`.
   - `memory/intrinsics.cpp` should remain the owner of memcpy/memset
     specialization.

4. Keep coordinator changes minimal.
   - Replace the moved call branch with a dispatch call.
   - Do not use this idea to redesign pointer-int cast handling, scalar binop
     handling, load/store, GEP, or alloca.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run relevant BIR/LIR-to-BIR call tests, including direct calls, indirect
     calls, byval/sret calls, and memcpy/memset runtime intrinsic calls.
   - Do not rewrite expectations.

## Acceptance Criteria

- `memory/coordinator.cpp` no longer contains the full `LirCallOp` lowering
  body.
- `memory/coordinator.cpp` dispatches calls through a small
  `lower_call_inst(...)` member call.
- Call lowering remains a `BirFunctionLowerer` member.
- Memory intrinsic specialization remains in `memory/intrinsics.cpp`.
- No new `.hpp` files are added.
- No call ABI, pointer provenance, or memory intrinsic semantics change.
- `c4c_codegen` builds.
- Relevant call and memory intrinsic tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign call ABI lowering.
- Do not split all call lowering into multiple files.
- Do not move memcpy/memset implementation back into call lowering.
- Do not change memory load/store/GEP behavior.
- Do not introduce `MemoryLoweringState` or a call-lowering state owner.

## Closure Note

Closed after the active runbook reached Step 5 readiness. The coordinator call
branch now dispatches through `lower_call_inst(...)`, call lowering remains a
`BirFunctionLowerer` member implemented in `calling.cpp`, memory intrinsic
specialization remains in `memory/intrinsics.cpp`, no new `.hpp` files were
added, and backend regression guard passed with no new failures.
