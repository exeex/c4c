# RV64 Post-F128-SoftFloat-Negation Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `235aaa2a`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane
  to shared f128 softfloat seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commits since then are the two landed shared-softfloat /
  negation packets that the checkpoint named
- commits since base: `2`
  (`4d8044fb rv64: wire shared f128 softfloat negation seam`,
  `29b719fd rv64: route f128 negation through shared softfloat seam`)

## Scope Reviewed

- `src/backend/f128_softfloat.cpp`
- `src/backend/f128_softfloat.hpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/{f128_softfloat,riscv/codegen/{f128,float_ops,comparison}}.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_f128_return_helper_symbol`
- `./build/backend_shared_util_tests riscv_translated_f128_neg_emits_bounded_text`
- `./build/backend_shared_util_tests riscv_translated_float_binop_f128_emits_bounded_text`

All four commands passed at this checkpoint. Current log paths:

- `rv64_f128_softfloat_build.log`
- `rv64_f128_header_test.log`
- `rv64_f128_neg_test.log`
- `rv64_f128_binop_test.log`

## Findings

- Medium: the routed broader shared-surface packet is landed and still aligned
  with the active plan. The tree now has a real shared
  `backend/f128_softfloat` seam for operand reload, negation, and the already
  referenced F128 binop orchestration, while RV64 exposes the minimal hook
  surface needed to make `emit_f128_operand_to_a0_a1(...)`,
  `emit_f128_neg_full(...)`, and `emit_f128_neg_impl(...)` truthful follow-ons
  without widening into memory or cast ownership
  ([`src/backend/f128_softfloat.cpp`](/workspaces/c4c/src/backend/f128_softfloat.cpp:43),
  [`src/backend/f128_softfloat.hpp`](/workspaces/c4c/src/backend/f128_softfloat.hpp:16),
  [`src/backend/riscv/codegen/f128.cpp`](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:83),
  [`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:81),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1150)).
- Medium: the implementation widened slightly beyond the narrow wording of the
  checkpoint by moving the existing RV64 F128 binop call path onto the shared
  soft-float orchestration at the same time, but that widening stays inside
  the same broader shared-surface route and is explicitly proven by focused
  coverage. This is still a coherent acceptance boundary rather than drift into
  a separate initiative
  ([`src/backend/f128_softfloat.cpp`](/workspaces/c4c/src/backend/f128_softfloat.cpp:69),
  [`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:57),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1190)).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still name the shared-softfloat / negation packet as the next action even
  though both execution commits are already in `HEAD`, so more execution would
  stack on top of an out-of-date route checkpoint
  ([`plan.md`](/workspaces/c4c/plan.md:49),
  [`todo.md`](/workspaces/c4c/todo.md:8)).
- Medium: the next truthful follow-on is the shared F128 comparison seam, not
  broader F128 load/store/cast work. RV64 `comparison.cpp` still throws on
  `IrType::F128`, while the Rust reference routes that entry directly through
  `backend::f128_softfloat::f128_cmp(...)`; by contrast, the memory and cast
  follow-ons still depend on a materially larger soft-float trait surface and
  tracked-source state that the live C++ seam does not model yet
  ([`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:56),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:139),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:435),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:480),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:669)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks the shared `backend/f128_softfloat.cpp` seam plus the minimal RV64
  `emit_f128_neg_full(...)` / `emit_f128_neg_impl(...)` hook surface complete,
  with the focused build/test proof lane above
- records that the shared seam now also owns the currently routed RV64 F128
  binop orchestration path already exercised by `float_ops.cpp`
- routes the next lifecycle stage to the next shared F128 follow-on:
  define the minimal additional `backend/f128_softfloat.cpp` orchestration and
  RV64 hook surface needed to make
  `src/backend/riscv/codegen/comparison.cpp::emit_f128_cmp_impl(...)`
  truthful

Keep these families parked:

- broader `memory.cpp` F128 store/load/store-with-offset/load-with-offset work
- `cast_ops.cpp` F128 cast dispatch
- broader `calls.cpp` outgoing-call lowering and F128/I128 result-store work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- integer-owner, variadic, and broader comparison follow-on work beyond the
  bounded F128 compare seam

## Rationale

The two execution commits after `235aaa2a` do the work the route asked for:
they create a real shared F128 soft-float boundary, move RV64 negation through
it, and preserve focused proof. The branch is therefore still on track, but
the runbook is stale because it still points at work already landed.

The next honest packet is not another negation tweak and not the much wider
F128 memory/cast surface. The next still-parked translated RV64 entry that
directly routes through the shared soft-float reference is
`emit_f128_cmp_impl(...)`, while the memory/cast follow-ons would require a
substantially larger trait/state expansion than the current seam exposes.
