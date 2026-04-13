# RV64 Post-ALU Integer-Owner Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `ae6fb0f4`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to alu integer-owner packet`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle checkpoint in `plan.md` history for the
  current active plan, and `HEAD` contains exactly one execution commit on top
  of that route selection
- commits since base: `1`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{alu,comparison,calls,globals,memory,emit}.rs`
- `tests/backend/backend_shared_util_tests.cpp`

## Validation Checked

- `scripts/plan_change_gap.sh`
- `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/`
- `git diff ae6fb0f4..HEAD`
- commit `0c8e04fe`

No new build was run for this review-only checkpoint.

## Findings

- Medium: canonical lifecycle state is now stale because the active ALU packet
  named by `plan.md` / `todo.md` has already landed. The current route is the
  first broader `alu.cpp` integer-owner packet centered on `IrBinOp`,
  `emit_int_binop_impl(...)`, and `emit_copy_i128_impl(...)`
  (`plan.md`, current target "lane status"; `todo.md`, current active item),
  and commit `0c8e04fe` implements that exact surface in
  `src/backend/riscv/codegen/alu.cpp:49-66`,
  `src/backend/riscv/codegen/riscv_codegen.hpp:330-336`, and focused shared
  util coverage in `tests/backend/backend_shared_util_tests.cpp`. The branch is
  therefore past the active packet definition even though canonical state still
  points at it.
- Medium: the landed ALU slice stayed within the route that `ae6fb0f4`
  requested. The diff from `ae6fb0f4..HEAD` is limited to the expected ALU
  owner methods and the minimum shared header/test exposure:
  `src/backend/riscv/codegen/alu.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`, and
  `tests/backend/backend_shared_util_tests.cpp`. There is no spillover into
  parked `calls.cpp`, `globals.cpp`, `memory.cpp`, or broader comparison work.
- Medium: the next truthful route is no longer another ALU packet. The ref RV64
  ALU inventory after the landed integer-owner body is exhausted at this seam:
  Rust `alu.rs` contains only unary helpers, `emit_int_binop_impl(...)`, and
  `emit_copy_i128_impl(...)`
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/alu.rs:9-74`), and all of
  those are now real in the C++ tree.
- Medium: the smallest remaining translated owner follow-on is the bounded
  integer-compare packet in `comparison.cpp`, but only if lifecycle state is
  rewritten to keep the broader comparison family parked around it. The live
  C++ file already covers float compare only
  (`src/backend/riscv/codegen/comparison.cpp:56-87`), while the ref Rust file
  shows the remaining comparison inventory split into
  `emit_int_cmp_impl(...)`, `emit_fused_cmp_branch_impl(...)`, and
  `emit_select_impl(...)`
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:36-136`).
  Of those, `emit_int_cmp_impl(...)` is the narrowest truthful next packet
  because it only needs operand loading plus sub-64-bit sign/zero extension
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/emit.rs:165-208`), all of
  which sits adjacent to already-landed `operand_to_t0(...)`,
  `store_t0_to(...)`, and frame-slot helpers
  (`src/backend/riscv/codegen/riscv_codegen.hpp:499-503`;
  `src/backend/riscv/codegen/returns.cpp:47-83`). By contrast, fused branch and
  select still widen into label generation, jump helpers, and reg-cache
  invalidation that are not part of the current shared RV64 surface.
- Medium: the other visible translated families should remain parked. Broader
  outgoing-call lowering is still explicitly deferred in
  `src/backend/riscv/codegen/calls.cpp:121-131`; TLS globals remains outside
  the current surface in `src/backend/riscv/codegen/globals.cpp:62-63`; and the
  remaining `memory.cpp` typed-indirect / broader GEP follow-on is still the
  broader parked family identified by current lifecycle state rather than a new
  smaller owner packet.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite canonical lifecycle state before more execution.

The new route should:

- mark commit `0c8e04fe` as completing the ALU integer-owner packet selected at
  `ae6fb0f4`
- stop issuing executor work from the stale ALU packet definition
- route the next truthful execution slice to a bounded
  `src/backend/riscv/codegen/comparison.cpp` integer-compare packet centered
  on:
  - the minimum shared compare-operand-load helper surface needed to mirror
    `emit_cmp_operand_load(...)`
  - `RiscvCodegen::emit_int_cmp_impl(...)`
  - only the strictly required declaration/build wiring and focused proof for
    that compare-owner entry

Keep these families parked:

- `emit_fused_cmp_branch_impl(...)`
- `emit_select_impl(...)`
- broader comparison follow-on beyond the integer-compare packet
- broader `calls.cpp` outgoing-call lowering and parked I128 / F128 result
  paths
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `memory.cpp` typed-indirect and broader GEP owner work

## Rationale

After `0c8e04fe`, the branch has not drifted; it has completed the route that
canonical state selected. The required action is a lifecycle rewrite, not
implementation repair. The next honest packet is the smallest remaining
translated owner body with adjacent support that does not reopen the broader
parked families, which is the integer-compare seam in `comparison.cpp` rather
than another ALU slice or a pivot into wider calls, TLS globals, or broader
memory work.
