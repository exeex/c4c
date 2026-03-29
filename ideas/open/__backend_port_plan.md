# Backend Port Roadmap

## Status

This file is the umbrella roadmap for the native backend effort.

Execution should usually activate one of the narrower backend ideas in `ideas/open/`.

## Goal

Port `ref/claudes-c-compiler/src/backend/` to `src/backend/` in C++ and make it consume the existing LIR in `src/codegen/lir/`.

Targets remain:

- AArch64
- x86-64
- rv64

Linux remains the execution target for produced binaries.

## Default Strategy

Default strategy: perform a mechanical C++ port of `ref/claudes-c-compiler/src/backend/` with minimal architectural deviation.

That means:

- prefer matching the ref backend's file structure, phase structure, and mechanism boundaries
- avoid redesigning shared abstractions up front
- avoid mixing cleanup with behavior changes
- treat the LIR attachment layer as the main intentional divergence from ref

## Historical Child Ideas

- `15_backend_x86_port_plan.md`
- `16_backend_x86_regalloc_peephole_enablement_plan.md`
- `17_backend_x86_local_memory_addressing_plan.md`
- `18_backend_x86_global_addressing_plan.md`
- `19_backend_x86_extern_global_array_addressing_plan.md`
- `20_backend_x86_global_char_pointer_diff_plan.md`
- `21_backend_x86_global_int_pointer_diff_plan.md`
- `22_backend_x86_global_int_pointer_roundtrip_plan.md`
- `24_backend_builtin_linker_x86_plan.md`
- `26_backend_builtin_assembler_rv64_plan.md`
- `27_backend_rv64_codegen_routing_plan.md`
- `28_backend_builtin_linker_rv64_plan.md`
- `29_backend_rv64_relocation_linking_plan.md`

All listed child ideas above are closed as of 2026-03-29.

## Next Child Idea To Activate

- `31_backend_x86_runtime_case_convergence_plan.md`
- This is the currently staged backend child for the remaining bounded x86 gap:
  promoting more real runtime `.c` cases from LLVM-text fallback to backend-owned asm.
- The first explicit target is `tests/c/internal/backend_case/call_helper.c`,
  followed by bounded parameter-passing and nearby compare/local/global slices.
- `30_backend_rv64_archive_linking_plan.md`
- This remains a separate staged RV64 child for the bounded archive-handling
  gap in the first helper-call static-link path.

## 2026-03-29 Audit Note

After closing the bounded RV64 assembler, codegen-routing, first linker, and
first relocation-linking children, the next missing backend-owned RV64
mechanism family is bounded archive-backed input loading for that existing
helper-call path:

- `src/backend/riscv/linker/` now supports the first relocation-bearing
  multi-object ET_EXEC helper-call slice when both inputs are explicit object
  files
- the next bounded gap is selecting one archive member that defines the
  unresolved helper symbol and feeding that object through the already-closed
  RV64 JAL relocation path
- broader global/data addressing, wider relocation coverage, and general
  archive expansion should remain deferred until later child ideas

For x86, the first backend-owned asm slice is closed, but focused runtime
verification now shows a different remaining gap:

- `return_zero.c`, `return_add.c`, and `local_array.c` already run through the
  backend-owned x86 asm path
- `call_helper.c` still falls back to LLVM text even when the runtime harness
  requests `BACKEND_OUTPUT_KIND=asm`
- the next x86 child should therefore converge more real runtime cases onto the
  backend-owned asm path before treating x86 as ready for broader assembler or
  linker follow-ons

Execution should continue through the staged child
`ideas/open/31_backend_x86_runtime_case_convergence_plan.md` for x86 or
`ideas/open/30_backend_rv64_archive_linking_plan.md` for RV64 rather than
activating this umbrella roadmap directly.

## 2026-03-29 Deactivation Note

The umbrella roadmap was briefly reactivated as a planning-only runbook after
closing the bounded RV64 archive-linking slice.

That active runbook is now intentionally parked again instead of being used as
the implementation queue.

Important handoff knowledge from the parked umbrella runbook:

- do not implement directly from this umbrella file
- the next x86 child is now clearer and narrower than the umbrella:
  `ideas/open/31_backend_x86_runtime_case_convergence_plan.md`
- the first explicit x86 convergence target is
  `tests/c/internal/backend_case/call_helper.c`, because focused validation now
  shows:
  - `return_zero.c`
  - `return_add.c`
  - `local_array.c`
  already stay on the backend-owned x86 asm path
- broader x86 assembler or linker follow-ons should remain deferred until this
  runtime-case convergence child is complete
- RV64 archive-backed linking remains a separate closed slice and should not
  keep the umbrella artificially active

## 2026-03-29 Staging Note

The planning-only umbrella runbook initially restated the already-closed RV64
helper-call relocation slice from
`ideas/closed/29_backend_rv64_relocation_linking_plan.md`.
That duplication was corrected during staging by promoting the next still-open
RV64 linker gap instead: archive-backed loading for the same bounded helper-call
path.

## Shared Guardrails

- Do not change existing LIR data structures just to make the next backend slice easier.
- Prefer an adapter or shim over an early LIR redesign.
- Keep target-specific code isolated by architecture.
- Prefer external toolchain fallback over partial built-in encoding/linking until the bounded backend-owned slice is explicit.
- Keep patches mechanical where possible so ref/backend diffs stay reviewable.
- One mechanism family per patch.
