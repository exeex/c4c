# RV64 Post-Integer-Compare Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `9af41f64`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-int-cmp route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that commit `c5a46529` completed the bounded integer-compare packet
  while intentionally pausing the lane for a broader-route decision
- commits since base: `0`

## Scope Reviewed

- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs`

## Findings

- Medium: the smallest truthful broader follow-on is the translated
  `comparison.cpp` control-flow seam, not a pivot to another parked family.
  The live RV64 comparison owner already carries the landed compare-operand
  loader plus `emit_int_cmp_impl(...)` and `emit_float_cmp_impl(...)`
  (`src/backend/riscv/codegen/comparison.cpp:14-176`), while the remaining
  Rust inventory in that same owner file is exactly
  `emit_fused_cmp_branch_impl(...)` plus `emit_select_impl(...)`
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:75-135`).
  Those two follow-ons directly consume the now-landed compare operand loading
  seam instead of requiring a fresh owner-family pivot.
- Medium: this comparison-control-flow route is broader than the exhausted
  helper lane, but still narrower than the parked call and memory families.
  The shared RV64 header does not yet expose any comparison-control-flow entry
  points beyond integer and float compare
  (`src/backend/riscv/codegen/riscv_codegen.hpp:481-509`), so this route must
  add the minimal declaration/build surface for label-based fused
  compare-and-branch plus select. Even so, that is materially smaller than the
  remaining call inventory, which immediately fans out into F128 pre-convert,
  stack-arg lowering, register-arg staging, and struct/I128/F128 cases
  (`src/backend/riscv/codegen/calls.cpp:121-131`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:29-220`).
- Medium: broader memory and TLS globals should remain parked. The current C++
  memory owner only makes the default and F128 load/store entry points real
  today (`src/backend/riscv/codegen/memory.cpp:59-143`), while the remaining
  translated memory inventory still widens into typed-indirect/GEP/stateful
  helpers with label-generation and register-cache machinery that the shared
  RV64 surface still does not model
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs:103-220`).
  TLS globals is still isolated and state-heavy rather than a better next
  route: the live C++ file explicitly keeps broader globals work parked on
  shared GOT/TLS state not present in the current surface
  (`src/backend/riscv/codegen/globals.cpp:62-63`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects this broader next
route:

- route the next execution packet to the translated
  `src/backend/riscv/codegen/comparison.cpp` control-flow seam centered on
  `emit_fused_cmp_branch_impl(...)` plus `emit_select_impl(...)`
- keep the shared RV64 delta minimal:
  add only the declaration/build/proof surface needed for those label-based
  comparison-control-flow entries and the minimum local label-generation
  support they require
- keep these families parked:
  broader outgoing-call lowering and I128/F128 call-result follow-on work,
  `globals.cpp::emit_tls_global_addr_impl(...)`, typed-indirect / broader GEP
  memory work, and any broader comparison-control-flow expansion beyond the
  bounded fused-branch/select route

## Rationale

The blocked post-int-compare checkpoint was correct, but the next truthful
route is now visible without guessing. The just-landed integer-compare packet
completed the last narrow helper-scale slice on the comparison lane, and the
remaining translated inventory in that owner file is the label-based
control-flow pair that naturally follows it. Choosing that pair next keeps the
lane adjacent to the newly landed compare-operand-load seam instead of
resetting into wider call-lowering, memory-state, or TLS work.

This is still a broader route, not another helper packet. It requires a small
shared-surface expansion for comparison-control-flow entry points and local
label handling, so canonical lifecycle state should be rewritten before more
execution. But it remains the narrowest honest next step now that the direct
integer-compare seam is complete.
