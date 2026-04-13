# RV64 Post-Intrinsics-X86-Fallback Broader Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `6ba9135e`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 intrinsics lane after x86 fallback block`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `6ba9135e` as the latest canonical
  lifecycle checkpoint, and that commit is the plan/todo rewrite that accepts
  the completed x86-only fallback block while explicitly blocking the RV64 lane
  for a fresh broader-route decision
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_intrinsics_x86_fallback_route_audit.md`
- `src/backend/backend.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/README.md`
- `src/backend/riscv/README.md`
- `tests/backend/backend_shared_util_tests.cpp`
- recent lifecycle history on `plan.md`, `todo.md`, and
  `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Findings

- Medium: the blocked post-intrinsics checkpoint is still truthful and there is
  no implementation drift to absorb before the next route choice.
  `scripts/plan_change_gap.sh` reports `last_plan_change_gap=0`, and the
  current route audit already records that the direct `emit_intrinsic_rv(...)`
  owner lane is consumed after commits `813bb64f` and `7c9e0776` rather than
  exposing another adjacent executor packet:
  [review/rv64_post_intrinsics_x86_fallback_route_audit.md](/workspaces/c4c/review/rv64_post_intrinsics_x86_fallback_route_audit.md:55),
  [review/rv64_post_intrinsics_x86_fallback_route_audit.md](/workspaces/c4c/review/rv64_post_intrinsics_x86_fallback_route_audit.md:86).
- Medium: `peephole.cpp` is still not the truthful next packet by itself, but
  the parked-family text now points at the broader route that *would* make it
  truthful. Canonical state explicitly says to keep `peephole.cpp` parked until
  a later native-emitter / backend-finalization route makes it the nearest
  continuation after the now-complete intrinsics lane:
  [plan.md](/workspaces/c4c/plan.md:143),
  [plan.md](/workspaces/c4c/plan.md:146),
  [todo.md](/workspaces/c4c/todo.md:119),
  [todo.md](/workspaces/c4c/todo.md:123).
- Medium: the real missing integration seam is now top-level RV64 backend
  consumption, not another owner-file packet inside the already-compiled
  codegen cluster. The RV64 docs describe a native pipeline with codegen,
  peephole, assembler, and linker, but `backend.cpp` still routes prepared
  LIR to LLVM text and BIR to textual fallback for `Target::Riscv64`, and the
  shared-util test suite still asserts that RV64 preparation stays untouched so
  the text fallback keeps ownership:
  [src/backend/riscv/README.md](/workspaces/c4c/src/backend/riscv/README.md:3),
  [src/backend/riscv/codegen/README.md](/workspaces/c4c/src/backend/riscv/codegen/README.md:34),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:65),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:89),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:8664).
- Medium: `emit.cpp` still names the broader `RiscvCodegen` /
  `CodegenState` surface and `ArchCodegen` implementation as missing, which is
  a more concrete remaining gap than reopening the stale memory route or
  inventing another abstract parked owner family:
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:2),
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:55).
- Medium: the other parked broader families are less truthful than a
  backend-finalization route right now.
  The stale `memory.cpp` typed-indirect / broader GEP route is already
  recorded as stale in canonical state; broader variadic, comparison, and
  calls follow-on work are still parked beyond previously accepted widened
  routes; `inline_asm.cpp` was already rejected as overlapping the landed
  asm-emitter helper surface; and the plan names broader atomics only as an
  inactive abstract follow-on after the direct atomics lane was exhausted,
  without identifying a nearer concrete shared-surface gap than the missing
  top-level native RV64 emission path:
  [plan.md](/workspaces/c4c/plan.md:134),
  [plan.md](/workspaces/c4c/plan.md:142),
  [plan.md](/workspaces/c4c/plan.md:143),
  [plan.md](/workspaces/c4c/plan.md:144),
  [review/rv64_post_stale_memory_route_broader_route_audit.md](/workspaces/c4c/review/rv64_post_stale_memory_route_broader_route_audit.md:18),
  [review/rv64_post_stale_memory_route_broader_route_audit.md](/workspaces/c4c/review/rv64_post_stale_memory_route_broader_route_audit.md:20),
  [review/rv64_post_stale_memory_route_broader_route_audit.md](/workspaces/c4c/review/rv64_post_stale_memory_route_broader_route_audit.md:27).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state selects
the next truthful broader route as a **native-emitter / backend-finalization
activation** for RV64, centered on:

- the missing shared `RiscvCodegen` / `CodegenState` / `ArchCodegen` surface
  called out in `src/backend/riscv/codegen/emit.cpp`
- top-level RV64 backend consumption in `src/backend/backend.cpp`, replacing
  the current LLVM-text / BIR fallback ownership for `Target::Riscv64`
- only then the first bounded native-emitter follow-on packet, which may
  legitimately bring `peephole.cpp` into scope because that route would finally
  consume emitted RV64 assembly text

Keep these families parked:

- the stale `memory.cpp` typed-indirect / broader GEP route
- broader variadic follow-on beyond the landed owner-body seam
- broader comparison follow-on beyond the landed
  `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route
- broader calls-owner follow-on beyond the complete
  `emit_call_store_result_impl(...)` wide-result route
- `inline_asm.cpp` until the backend-finalization route needs more than the
  landed asm-emitter helper surface
- broader atomics follow-on unless the backend-finalization route exposes a
  concrete remaining atomics gap that is smaller than the missing native
  emission seam

## Rationale

The branch is not drifting. The current blocked checkpoint correctly records
that the direct intrinsics owner lane is complete. The route problem is that
continued owner-file execution would no longer move the integrated RV64 backend
toward real consumption: the repo already compiles a broad RV64 codegen
cluster, but the backend still does not use it for native emission.

That makes a backend-finalization activation more truthful than reopening the
stale memory route, guessing at another broad calls/comparison/variadic packet,
or landing `peephole.cpp` in isolation. `peephole.cpp` becomes honest only as
part of the larger route that first gives RV64 a native emitted assembly path
to optimize.
