# Backend Regalloc And Peephole Port Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/02_backend_aarch64_port_plan.md`

Should precede:

- `ideas/open/04_backend_binary_utils_contract_plan.md`
- `ideas/open/05_backend_builtin_assembler_boundary_plan.md`
- `ideas/open/06_backend_builtin_assembler_aarch64_plan.md`
- `ideas/open/07_backend_linker_object_io_plan.md`
- `ideas/open/08_backend_builtin_linker_aarch64_plan.md`

## Goal

Port the shared register-allocation, liveness, and peephole/cleanup mechanisms from `ref/claudes-c-compiler/src/backend/` into C++, starting by making the mirrored shared files compile and connect cleanly to the AArch64 bring-up path.

## Simplified Staging Note

This idea is now mainly a staging bucket for the shared backend layers that AArch64 will need next.

It does not need to stay permanently separate if execution shows that the shared-backend compile work and the AArch64 integration work are better handled inside one active runbook.

## Why This Sits Between AArch64 And x86-64

- the first AArch64 bring-up is the cheapest place to prove the existing backend boundary
- register allocation and late cleanup are shared backend mechanisms, not only AArch64 details
- porting those shared layers once before x86-64 and rv64 reduces duplicated target-specific workaround work
- later target ports should inherit the same ref-shaped liveness/regalloc/cleanup structure instead of each reinventing a stack-only path

## Porting Style

- prefer mechanical translation from Rust to C++
- keep file and phase boundaries close to ref
- treat target-specific register sets and ABI quirks as thin per-target inputs to shared machinery
- avoid mixing new optimization ideas with the ref-matching port

## Primary Ref Surfaces

- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`
- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/state.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/regalloc_helpers.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/analysis.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/slot_assignment.rs`
- `ref/claudes-c-compiler/src/backend/peephole_common.rs`
- target-local prologue/emit integration points under:
  `ref/claudes-c-compiler/src/backend/arm/`,
  `ref/claudes-c-compiler/src/backend/x86/`,
  `ref/claudes-c-compiler/src/backend/riscv/`
- especially:
  `ref/claudes-c-compiler/src/backend/arm/codegen/prologue.rs`,
  `ref/claudes-c-compiler/src/backend/arm/codegen/emit.rs`,
  `ref/claudes-c-compiler/src/backend/arm/codegen/peephole.rs`,
  `ref/claudes-c-compiler/src/backend/x86/codegen/peephole/README.md`

## Ref Shape To Preserve

- keep the shared liveness and linear-scan allocator in top-level backend code rather than burying them inside one target
- keep stack-layout cooperation explicit: regalloc should feed frame sizing and callee-saved-save decisions, not become a later ad hoc cleanup
- preserve the ref split between:
  liveness computation,
  register assignment,
  stack-layout helpers,
  and target-local final cleanup
- treat the peephole layer as post-codegen assembly-text cleanup, not as a rewrite of instruction selection

## Scope

### Shared bring-up

- make the mirrored shared files under `src/backend/`, `src/backend/stack_layout/`, and nearby dependencies include-clean enough to compile as a unit
- port backward-dataflow liveness analysis and interval computation from `liveness.rs`
- port the shared linear-scan allocator from `regalloc.rs`, including:
  call-spanning interval detection,
  separate callee-saved and caller-saved pools,
  spillover into remaining callee-saved registers,
  loop-depth-weighted use counts,
  and value-eligibility filtering for non-GPR values
- port the helper boundary in `stack_layout/regalloc_helpers.rs` so frame sizing can reuse computed liveness and assigned physical registers
- preserve the ref `RegAllocResult` shape closely enough that target prologue/epilogue code can decide which callee-saved registers actually need save/restore
- port only the smallest ref-shaped peephole or cleanup passes required for correctness or practical code quality on the AArch64 path first

### Target integration

- thread the new shared machinery into the AArch64 backend first
- mirror the ref boundary where `emit` and `prologue` consume register assignments and used-register sets
- keep x86-64 and rv64 integration points ready to reuse the same shared layers
- preserve explicit target ownership of register pools, caller/callee-saved sets, and ABI-driven clobber rules

## Acceptance Stages

### Stage 1: Shared tree compiles

- top-level shared backend files for liveness/regalloc/stack-layout can be included together and built
- temporary shims are acceptable if they preserve the ref-shaped file boundaries

### Stage 2: AArch64 can include the shared layers

- AArch64 codegen files can include and reference the shared regalloc/liveness interfaces
- backend driver wiring no longer treats these shared layers as disconnected scaffolding

### Stage 3: Behavior starts tightening

- frame sizing, used-register tracking, and minimal peephole cleanup begin affecting real AArch64 codegen behavior
- targeted tests can start validating the new shared machinery instead of only compile-time wiring

## Suggested Execution Order

1. port `liveness.rs` and confirm interval shape on representative multi-block and loop-heavy functions
2. port `regalloc.rs` with the same eligibility rules and register-pool split used by ref
3. port `stack_layout/regalloc_helpers.rs` and thread cached liveness into frame sizing
4. wire the result into AArch64 prologue and emit paths so used callee-saved registers drive save/restore emission
5. add only the minimum AArch64 peephole cleanup needed to remove obviously redundant stack traffic exposed by the accumulator model
6. leave broader x86-style global peephole passes as a later follow-on unless AArch64 correctness depends on them

## Explicit Non-Goals

- full SSA redesign
- novel optimizer work beyond the ref backend's existing cleanup shape
- built-in assembler
- built-in linker
- broad target-generic backend refactors unrelated to regalloc or late cleanup

## Validation

- first validation gate: shared regalloc/liveness/stack-layout files compile and are reachable from the AArch64 backend build
- second validation gate: AArch64 prologue/epilogue save-restore behavior is driven by actual used callee-saved sets instead of hardcoded saves
- later validation: loop-heavy and multi-block functions show ref-shaped interval and assignment behavior in targeted tests
- x86-64 and rv64 plans can depend on this shared layer instead of each carrying their own first regalloc port

## Good First Patch

Make the minimum shared liveness/regalloc helper boundary compile and be includable from the AArch64 backend tree before tightening any runtime behavior.
