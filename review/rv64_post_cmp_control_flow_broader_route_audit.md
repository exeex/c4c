# RV64 Post-Comparison-Control-Flow Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `41f386c1`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane
  pending post-cmp-control-flow route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that commit `c1d152ec` completed the broader comparison-control-flow
  packet while intentionally pausing the lane for a fresh broader-route
  decision
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/assembler/encoder/base.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`
- recent route-audit artifacts under `review/`

## Findings

- Medium: the current branch is aligned with the active runbook and is still
  sitting exactly on the blocked checkpoint that was meant to pause execution
  after the comparison-control-flow seam landed. There is no implementation
  drift to absorb before the next route is chosen.
- Medium: `calls.cpp` is still a materially broader follow-on than the next
  honest packet. The live owner file explicitly leaves only the wider
  `emit_call_f128_pre_convert_impl(...)`,
  `emit_call_stack_args_impl(...)`, and
  `emit_call_reg_args_impl(...)` inventory parked behind a broader shared
  surface rather than another one-entry seam
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:121)).
- Medium: `memory.cpp` no longer presents the smallest visible gap. The live
  owner already carries the translated default load/store, const-offset,
  indirect, and over-aligned paths through the shared `SlotAddr` surface, so
  pivoting back into memory would be a broader family reset rather than the
  nearest missing owner entry
  ([`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:59)).
- Medium: `globals.cpp` now exposes the smallest truthful next seam. The live
  RV64 header exports `emit_global_addr_impl(...)` and `emit_label_addr_impl(...)`
  but not `emit_tls_global_addr_impl(...)`, while the translated Rust owner
  shows that TLS address materialization itself is a single Local-Exec entry
  that still terminates at `store_t0_to(dest)` instead of requiring the wider
  call-lowering surface
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:481),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:34),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:27)).
- Medium: the shared backend already models the critical assembler/linker side
  of this TLS route. The RV64 assembler encoder recognizes `%tprel_hi(...)`,
  `%tprel_lo(...)`, and `%tprel_add(...)`, so the next packet does not need to
  invent a new relocation family before the translated TLS owner body can be
  made truthful
  ([`src/backend/riscv/assembler/encoder/base.cpp`](/workspaces/c4c/src/backend/riscv/assembler/encoder/base.cpp:330)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects this broader next
route:

- route the next execution packet to the translated
  `src/backend/riscv/codegen/globals.cpp::emit_tls_global_addr_impl(...)`
  seam
- keep the shared RV64 delta minimal:
  add only the header/export surface, owner-body implementation, and focused
  shared-util proof needed for the Local-Exec TLS address-materialization path
- keep these families parked:
  broader outgoing-call lowering and I128/F128 call-result follow-on work,
  broader memory follow-on beyond the already-landed load/store surface, and
  any broader comparison expansion beyond the now-landed
  fused-branch/select route

## Rationale

The blocked post-comparison-control-flow checkpoint was correct, but the next
truthful route is now visible without guessing another wide family reset. The
comparison lane is exhausted, `calls.cpp` still fans out into multi-method ABI
staging, and the live memory owner is already materially further along than the
historical parked description implies.

By contrast, `globals.cpp` is down to one translated owner entry whose emitted
shape is already explicit in the Rust source and whose relocation modifiers are
already understood by the RV64 assembler layer. That makes the TLS globals seam
the narrowest honest next packet on the current shared surface.
