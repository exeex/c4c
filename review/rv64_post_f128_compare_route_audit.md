# RV64 Post-F128-Compare Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `6c4b0399`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane
  to shared f128 compare seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution commit since then is the compare packet that checkpoint named
- commits since base: `1`
  (`a439d33b rv64: wire shared f128 compare seam`)

## Scope Reviewed

- `src/backend/f128_softfloat.cpp`
- `src/backend/f128_softfloat.hpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/f128_softfloat.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_f128_compare_surface`
- `./build/backend_shared_util_tests riscv_translated_f128_cmp_emits_bounded_text`

Current log paths:

- `rv64_f128_cmp_build.log`
- `rv64_f128_cmp_header_test.log`
- `rv64_f128_cmp_test.log`

## Findings

- Medium: the compare packet landed cleanly and stays aligned with the active
  runbook. The shared soft-float seam now owns the routed F128 compare libcall
  mapping and bool-materialization contract, while `comparison.cpp` stops
  throwing on `IrType::F128` and routes that case through the shared helper
  exactly where the Rust reference does
  ([`src/backend/f128_softfloat.cpp`](/workspaces/c4c/src/backend/f128_softfloat.cpp:42),
  [`src/backend/f128_softfloat.cpp`](/workspaces/c4c/src/backend/f128_softfloat.cpp:105),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:56),
  [`src/backend/riscv/codegen/f128.cpp`](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:125),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:435),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:140)).
- Medium: proof for the routed seam is appropriately focused. The header export
  and emitted-text tests cover the new owner entry and the bounded compare
  orchestration without claiming broader F128 memory, cast, or call lowering
  coverage
  ([`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:276),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1296)).
- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still name the compare seam as the next action even though commit `a439d33b`
  already landed it, so more execution would stack on top of an out-of-date
  checkpoint
  ([`plan.md`](/workspaces/c4c/plan.md:49),
  [`todo.md`](/workspaces/c4c/todo.md:8)).
- Medium: there is no equally narrow follow-on left on the current seam. In the
  reference comparison owner, the remaining visible work after F128 compare is
  integer compare, fused cmp+branch, and select, which are the broader parked
  comparison-owner bodies rather than another shared-softfloat packet; in the
  shared F128 reference, the next surfaces after compare are store/load and
  cast dispatch, which are materially broader than the current compare route
  and match the still-parked RV64 `memory.cpp` / `cast_ops.cpp` families
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:34),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:475),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:665),
  [`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:58),
  [`src/backend/riscv/codegen/cast_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/cast_ops.cpp:201)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `a439d33b` as the landed bounded shared F128 compare seam with
  the focused proof lane above
- records that `comparison.cpp::emit_f128_cmp_impl(...)` is now truthful via
  the shared `backend/f128_softfloat.cpp` compare orchestration
- blocks the lane pending a fresh broader-route decision instead of issuing
  another executor packet immediately, because the remaining visible follow-on
  work now widens into broader shared F128 store/load or cast dispatch, or
  into the parked integer/fused-branch/select comparison-owner bodies

Keep these families parked:

- broader `memory.cpp` F128 store/load/store-with-offset/load-with-offset work
- `cast_ops.cpp` F128 cast dispatch
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `calls.cpp` outgoing-call lowering and F128/I128 result-store work
- integer-owner, fused-branch, select, and broader comparison follow-on work

## Rationale

The only execution commit since `6c4b0399` does exactly what that lifecycle
checkpoint asked for: it turns the parked RV64 F128 compare path into a real
shared-softfloat route and proves it with the named focused tests. This is a
coherent acceptance boundary, not drift.

The next honest action is not to guess another tiny execution packet. The
comparison owner has no remaining helper-sized soft-float follow-on after the
F128 compare entry, while the shared F128 reference moves next into much wider
store/load and cast orchestration that the current RV64 C++ surface still keeps
parked. That makes this another lifecycle rewrite point, not another direct
executor packet.
