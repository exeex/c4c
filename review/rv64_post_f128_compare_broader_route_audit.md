# RV64 Post-F128-Compare Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `cc204f0c`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending broader post-f128 route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that the F128 compare packet landed cleanly
- commits since base: `0`

## Scope Reviewed

- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/f128_softfloat.cpp`
- `src/backend/f128_softfloat.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`
- `ref/claudes-c-compiler/src/backend/f128_softfloat.rs`

## Findings

- Medium: the smallest truthful broader follow-on is the shared F128 memory
  seam, not the cast lane. The Rust RV64 memory owner already routes the four
  F128 store/load entry points directly into the shared soft-float layer,
  while the current C++ RV64 memory owner still returns early for `IrType::F128`
  in exactly those four entry points
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs:11`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs:19`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs:27`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs:60`,
  `src/backend/riscv/codegen/memory.cpp:58`,
  `src/backend/riscv/codegen/memory.cpp:66`,
  `src/backend/riscv/codegen/memory.cpp:74`,
  `src/backend/riscv/codegen/memory.cpp:108`).
- Medium: the cast lane is materially wider. The translated RV64 cast owner is
  still parked as commented inventory, so choosing F128 cast dispatch next
  would require reviving a broader owner file at the same time as extending
  the shared soft-float surface
  (`src/backend/riscv/codegen/cast_ops.cpp:190`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs:190`,
  `ref/claudes-c-compiler/src/backend/f128_softfloat.rs:665`).
- Medium: the shared F128 layer already contains the right integration shape
  for this broader route. It owns operand loading, negation, binops, and
  compare orchestration today, so extending it next to the store/load dispatch
  family keeps the lane on the same shared seam instead of jumping to a new
  owner family
  (`src/backend/f128_softfloat.cpp:60`,
  `src/backend/f128_softfloat.cpp:72`,
  `src/backend/f128_softfloat.cpp:86`,
  `src/backend/f128_softfloat.cpp:105`,
  `ref/claudes-c-compiler/src/backend/f128_softfloat.rs:475`,
  `ref/claudes-c-compiler/src/backend/f128_softfloat.rs:516`,
  `ref/claudes-c-compiler/src/backend/f128_softfloat.rs:560`,
  `ref/claudes-c-compiler/src/backend/f128_softfloat.rs:607`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `route next lifecycle slice to shared F128 memory seam`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects this broader next
route:

- route the next execution packet to the shared `backend/f128_softfloat.cpp`
  F128 memory seam:
  `f128_emit_store(...)`, `f128_emit_load(...)`,
  `f128_emit_store_with_offset(...)`, and
  `f128_emit_load_with_offset(...)`
- keep the RV64 owner delta minimal:
  wire only the `memory.cpp` F128 entry points and the minimum hook surface
  needed for the shared dispatch to resolve `SlotAddr`, move 16-byte values,
  and store/load through the existing RV64 address register conventions
- keep these families parked:
  `cast_ops.cpp` F128 cast dispatch, integer/fused-branch/select comparison
  owner work, TLS globals, broader outgoing-call lowering, typed indirect
  memory owner work, broader GEP owner work, and broader shared RV64 state
  expansion beyond what the shared F128 memory seam proves necessary

## Rationale

The blocked post-compare checkpoint was correct, but the next truthful route is
visible now without guessing. The RV64 translated memory owner already shows
the exact four F128 entry points that should delegate into shared soft-float,
and the C++ owner still no-ops those same branches today. That makes shared
F128 store/load dispatch the narrowest broader route that advances real
translated inventory.

Choosing casts next would widen too quickly. The RV64 cast owner is not yet a
live translated seam in C++, while the shared soft-float memory family is the
direct continuation of the same shared boundary that already absorbed negation,
binops, and compare.
