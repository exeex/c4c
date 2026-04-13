# RV64 Post-Native-Emitter-Activation Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `a5e95e7e`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to native emitter activation`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `a5e95e7e` as the latest canonical
  lifecycle checkpoint, and that commit is the rewrite that selected the
  current active native-emitter / backend-finalization activation packet
- commits since base: `1`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/backend.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/emit.hpp`
- `src/backend/riscv/codegen/peephole.cpp`
- `tests/backend/backend_bir_pipeline_tests.cpp`

## Validation Checked

- `rv64_native_emitter_activation_build_supervisor.log`
- `rv64_native_emitter_activation_default_path_supervisor.log`
- `rv64_native_emitter_activation_probe_supervisor.log`
- `rv64_native_emitter_activation_route_supervisor.log`

## Findings

- Medium: canonical lifecycle state is stale again. `plan.md` and `todo.md`
  still name the first RV64 native-emitter activation slice as active even
  though commit `85fcb363` has already landed the named `try_emit_prepared_lir_module(...)`
  seam, the shared backend hook, and the focused route tests for that exact
  packet:
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:53),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:84),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:2007).
- Medium: the landed packet matches the active route and does not justify a
  blocked checkpoint. The new RV64 seam stays activation-sized by accepting
  only the bounded tiny-add prepared-LIR slice, and the shared backend now
  consumes that seam for fresh RV64 LIR while leaving direct BIR and
  unsupported LIR on their prior text fallbacks:
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:21),
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:53),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:84),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:237),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:1895),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:4177).
- Medium: the route text’s condition for bringing `peephole.cpp` into scope is
  now satisfied. Current lifecycle state explicitly parks `peephole.cpp`
  unless the native-emitter / backend-finalization route exposes a concrete
  emitted-assembly seam, and commit `85fcb363` now does exactly that by
  producing native RV64 asm text for one bounded slice and routing shared
  backend ownership through it:
  [todo.md](/workspaces/c4c/todo.md:138),
  [todo.md](/workspaces/c4c/todo.md:139),
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:59),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:85).
- Medium: the nearest truthful continuation is a narrowed backend-finalization
  follow-on, not a broader emit.cpp support grab. `backend.cpp` already has an
  x86-specific finalization hook that routes native text through peephole
  optimization, while RV64 still returns raw emitted text even though
  `peephole.cpp` already exports a concrete optimizer entrypoint:
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:28),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:73),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:87),
  [src/backend/riscv/codegen/peephole.cpp](/workspaces/c4c/src/backend/riscv/codegen/peephole.cpp:1191).
- Medium: widening immediately into more native emit patterns would be less
  truthful than consuming the already-landed finalization surface. The active
  broader route was selected as a native-emitter / backend-finalization
  activation, not as a free-form expansion of `emit.cpp`, and the plan text
  already framed the first packet as the minimum entry seam before the next
  bounded continuation:
  [plan.md](/workspaces/c4c/plan.md:136),
  [plan.md](/workspaces/c4c/plan.md:139),
  [plan.md](/workspaces/c4c/plan.md:140).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `85fcb363` as the landed first RV64 native-emitter activation
  slice with the focused supervisor proof lane above
- records that the native entry seam and shared backend routing hook are now
  real for the bounded tiny-add prepared-LIR slice
- narrows the next execution packet to the first bounded **RV64
  backend-finalization follow-on** on the same route:
  expose a small shared RV64 peephole/finalization surface and route the
  newly native-emitted RV64 asm through `peephole_optimize(...)` for the
  supported native slice, while leaving unsupported RV64 modules and direct
  BIR on their existing text fallback paths

Keep these families parked:

- broader emit.cpp support expansion beyond the landed tiny-add slice
- assembler/linker finalization work
- the stale `memory.cpp` typed-indirect / broader GEP route
- broader variadic, comparison, calls, and atomics follow-on work
- `inline_asm.cpp`

## Rationale

The branch is not drifting. Commit `85fcb363` does exactly what the latest
canonical route asked for by landing the first native-emitter activation seam
and proving that the shared backend can consume it for one bounded RV64 LIR
slice.

That new seam changes the truthfulness of the parked-family list. Before this
commit, `peephole.cpp` was still premature because RV64 had no native emitted
assembly path to optimize. After this commit, RV64 now emits real assembly for
the bounded tiny-add slice, and `backend.cpp` already contains the cross-target
pattern for backend finalization through x86. The narrowest honest continuation
is therefore the first RV64 backend-finalization/peephole packet on the same
route, not another blocked checkpoint and not a broader emit.cpp capability
grab.
