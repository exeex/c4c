# RV64 Post-Native-Peephole Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `2872d600`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 native emitter route to peephole finalization`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `2872d600` as the latest canonical
  lifecycle checkpoint, and that commit is the rewrite that selected the
  current active RV64 backend-finalization / peephole packet
- commits since base: `1`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `CMakeLists.txt`
- `src/backend/backend.cpp`
- `src/backend/riscv/codegen/emit.hpp`
- `src/backend/riscv/codegen/peephole.cpp`
- `tests/backend/backend_bir_pipeline_tests.cpp`

## Validation Checked

- `./build/backend_shared_util_tests backend`
- `./build/backend_bir_tests test_backend_default_path_uses_riscv_native_entry_when_bir_pipeline_is_not_selected`
- `./build/backend_bir_tests test_backend_riscv64_supported_lir_uses_native_route_while_direct_bir_stays_on_text_route`
- `./build/backend_bir_tests test_backend_riscv64_path_keeps_unsupported_lir_on_llvm_text_surface`
- `./build/backend_bir_tests test_backend_riscv64_path_keeps_unsupported_direct_lir_on_llvm_text_surface`
- `./build/backend_bir_tests test_backend_bir_pipeline_selection_only_applies_at_lir_entry_input`
- `./build/backend_bir_tests test_backend_lowered_riscv_passthrough_is_detached_from_lir_metadata`

## Findings

- Medium: canonical lifecycle state is stale again. The active plan and todo
  still name the RV64 backend-finalization / peephole packet as the current
  work item even though commit `240f9578` has already landed the exact
  routing they describe:
  [plan.md](/workspaces/c4c/plan.md:146),
  [plan.md](/workspaces/c4c/plan.md:149),
  [todo.md](/workspaces/c4c/todo.md:81),
  [todo.md](/workspaces/c4c/todo.md:89),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:91),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:249).
- Medium: the landed diff matches the active packet truthfully. RV64 now has a
  minimal shared finalization seam, the peephole unit is wired into both main
  and backend-test builds, and the touched backend tests assert that
  supported native-emitted LIR is returned only after
  `peephole_optimize(...)` while unsupported RV64 LIR and direct BIR keep
  their prior fallback surfaces:
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:238),
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:480),
  [src/backend/riscv/codegen/emit.hpp](/workspaces/c4c/src/backend/riscv/codegen/emit.hpp:12),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:35),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:1896),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:4189).
- Medium: there is no equally narrow same-route executor follow-on left after
  `240f9578`. The only two RV64 native-entry call sites now both consume the
  same finalization seam, so more work on this branch would widen into broader
  `emit.cpp` support expansion or assembler/linker finalization, both of which
  the current plan explicitly parks rather than exposes as another bounded
  packet:
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:73),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:240),
  [plan.md](/workspaces/c4c/plan.md:163),
  [plan.md](/workspaces/c4c/plan.md:166),
  [todo.md](/workspaces/c4c/todo.md:148),
  [todo.md](/workspaces/c4c/todo.md:150).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks commit `240f9578` as the landed bounded RV64 backend-finalization /
  peephole packet with the focused backend proof above
- records that supported RV64 native-emitted tiny-add LIR now routes through
  the shared RV64 peephole finalization seam while unsupported RV64 LIR and
  direct BIR stay on their existing text fallback surfaces
- marks the current lane as blocked pending a fresh broader-route decision
  rather than another executor packet, because no equally narrow direct
  follow-on remains on the same native-emitter / backend-finalization route

Keep these families parked:

- broader `emit.cpp` support expansion beyond the bounded tiny-add slice
- assembler/linker finalization work
- the stale `memory.cpp` typed-indirect / broader GEP route
- broader variadic, comparison, calls, and atomics follow-on work
- `inline_asm.cpp`

## Rationale

This branch is still aligned with the accepted RV64 native-emitter /
backend-finalization route. Commit `240f9578` does exactly what the latest
canonical packet asked for: it finalizes native-emitted RV64 assembly through
the already-landed peephole surface without changing fallback ownership for
unsupported RV64 LIR or direct BIR input.

That also exhausts the currently selected packet family. The backend now uses
the same RV64 finalization seam at both bounded native-entry sites, so the next
truthful continuation is not another tiny same-route packet. More execution now
needs a fresh broader-route choice, and lifecycle should say so explicitly
before another executor handoff.
