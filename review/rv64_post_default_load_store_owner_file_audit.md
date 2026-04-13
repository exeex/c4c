# RV64 Post-Default-Load-Store Owner-File Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `f99ff01b`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-default-load-store audit`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the current active plan, and
  `HEAD` has not advanced past that block-before-routing checkpoint
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/x86/codegen/{x86_codegen.hpp,shared_call_support.cpp}`
- recent route-audit artifacts under `review/`

## Findings

- Medium: the current branch is aligned with the active runbook and is still
  sitting exactly on the lifecycle block that was meant to pause execution for
  this owner-file audit. There is no implementation drift to absorb before the
  next route is chosen.
- Medium: the remaining visible `memory.cpp` work is not another bounded
  follow-on on the just-landed scalar default-load/store surface. The live
  owner entries now stop at the scalar non-`F128` path, while the still-comment
  reference body immediately widens into shared soft-float dispatch, typed
  indirect addressing, and broader GEP/state expansion.
- Medium: the broadest obvious escape hatch is still too wide. `f128.cpp`,
  `float_ops.cpp`, and `comparison.cpp` all keep the next visible F128 work
  parked behind the missing shared `backend/f128_softfloat.cpp` orchestration
  and missing RV64 state needed to track full-precision sources and constants.
- Medium: `globals.cpp` now exposes the smallest truthful next seam. The
  already-landed `emit_label_addr_impl(...)` entry and preparatory
  `store_t0_to(...)` helper mean `emit_global_addr_impl(...)` only needs a
  minimal shared GOT-address query on `RiscvCodegenState` plus the bounded
  owner declaration/export surface; it does not require the broader RV64
  memory or F128 state.
- Medium: the x86 backend already shows the shape of that missing shared state.
  `mark_needs_got_for_addr(...)` / `needs_got_for_addr(...)` live as a narrow
  state surface there, which makes RV64 global-address routing a smaller and
  more concrete next packet than reopening the parked F128 or broader memory
  lanes.
- Medium: `emit_tls_global_addr_impl(...)` should stay parked. TLS relocation
  emission is visible in the translated owner file, but it is not required to
  land the narrower GOT-aware `emit_global_addr_impl(...)` seam and would widen
  the packet beyond the minimal state addition.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

The next bounded execution packet should move into
`src/backend/riscv/codegen/globals.cpp`, but only for the translated
`emit_global_addr_impl(...)` seam plus the smallest shared state it actually
needs:

- shared `RiscvCodegenState` GOT-address tracking surface
  (`mark_needs_got_for_addr(...)` / `needs_got_for_addr(...)` or equivalent)
- the shared RV64 header declaration/export needed for
  `emit_global_addr_impl(...)`
- the translated `globals.cpp::emit_global_addr_impl(...)` owner body
- focused shared-util coverage for local-address vs GOT-address emission

Keep these families parked:

- `globals.cpp::emit_tls_global_addr_impl(...)`
- remaining `memory.cpp` follow-on beyond the landed scalar default
  load/store packet
- broader F128 soft-float routing in `f128.cpp`, `float_ops.cpp`, and
  `comparison.cpp`
- remaining `alu.cpp`, `calls.cpp`, and `variadic.cpp` broader owner work

## Rationale

This checkpoint is another route-selection event, not a drift event. The
post-default-load/store block said to stop and run a fresh audit, and the repo
is still exactly at that pause point. The audit result is that the memory lane
has again reached a boundary where the next visible work would hide a broader
shared-state expansion inside an implementation packet.

`globals.cpp::emit_global_addr_impl(...)` is the smallest remaining owner-file
entry that now has a credible supporting surface. The previously missing
result-store hook is already real, and the only newly justified shared addition
is a narrow GOT-address query on RV64 state, with a clear x86 shape reference.
That makes it a tighter next packet than reopening F128 soft-float orchestration
or pretending the remaining memory follow-on is still on the same bounded seam.
