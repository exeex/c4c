# RV64 Post-Native-Peephole Broader Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `e81f5721`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 native emitter lane after peephole packet`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `e81f5721` as the latest canonical
  lifecycle checkpoint, and that commit is the rewrite that accepts the
  bounded RV64 native-emitter / peephole packet while explicitly blocking the
  lane for a fresh broader-route decision
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_native_peephole_route_audit.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `CMakeLists.txt`
- `src/backend/backend.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/README.md`
- `src/backend/riscv/README.md`
- `src/backend/riscv/assembler/mod.cpp`
- `tests/backend/backend_bir_pipeline_tests.cpp`
- `tests/backend/backend_shared_util_tests.cpp`

## Findings

- Medium: the blocked post-native-peephole checkpoint is still truthful and
  there is no implementation drift to absorb before the next route choice.
  `scripts/plan_change_gap.sh` reports `last_plan_change_gap=0`, and the
  current route audit already records that commit `240f9578` exhausted the
  bounded native-emitter / peephole packet rather than exposing another
  same-route executor slice:
  [review/rv64_post_native_peephole_route_audit.md](/workspaces/c4c/review/rv64_post_native_peephole_route_audit.md:9),
  [review/rv64_post_native_peephole_route_audit.md](/workspaces/c4c/review/rv64_post_native_peephole_route_audit.md:48).
- Medium: the next concrete RV64 integration gap is now binary-utils
  consumption, not another `emit.cpp` packet. The backend already routes the
  bounded prepared-LIR slice through native RV64 emission and shared peephole
  finalization, but `assemble_target_lir_module(...)` still special-cases
  `Target::Riscv64` by returning staged text with `object_emitted = false`
  instead of consuming a built-in RV64 assembler surface:
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:32),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:226),
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:229).
- Medium: the translated RV64 assembler and linker inventory exists in-tree,
  but canonical build wiring still stops at codegen. `src/backend/riscv/`
  documentation describes a full codegen -> assembler -> linker pipeline and
  the translated assembler/linker source trees are present, yet the active
  CMake backend source lists include only `src/backend/riscv/codegen/*.cpp`
  and omit `src/backend/riscv/assembler/*.cpp` and
  `src/backend/riscv/linker/*.cpp` entirely:
  [src/backend/riscv/README.md](/workspaces/c4c/src/backend/riscv/README.md:3),
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:221),
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:463),
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:1).
- Medium: the current public proof surfaces also show that RV64 ownership
  still ends at emitted text. Shared-util tests verify x86 and aarch64 wrapper
  parity for `assemble_target_lir_module(...)`, but there is no equivalent
  RV64 assembly-wrapper proof, and the existing RV64 preparation test still
  documents text-fallback ownership rather than object emission:
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:8671),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9057).
- Medium: broader `emit.cpp` support expansion is now a less truthful next
  route than RV64 assembler activation. `emit.cpp` still calls out the missing
  wider `RiscvCodegen` / `CodegenState` surface, but the newly-landed backend
  entry already consumes the only bounded native-emittable prepared-LIR slice;
  widening there would reopen broad codegen inventory while leaving the more
  immediate top-level binary-utils handoff gap untouched:
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:2),
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:50),
  [tests/backend/backend_bir_pipeline_tests.cpp](/workspaces/c4c/tests/backend/backend_bir_pipeline_tests.cpp:1896).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state selects
the next truthful broader route as an **RV64 assembler / binary-utils
activation** centered on:

- the missing top-level RV64 built-in assembler consumption in
  `src/backend/backend.cpp::assemble_target_lir_module(...)`
- the minimum shared/public assembler entry surface needed to stage emitted
  RV64 asm into an object file rather than stopping at text with
  `object_emitted = false`
- the build activation needed to make the translated RV64 assembler inventory
  real in the active test/build graph before any later linker follow-on is
  considered

The first bounded packet on that route should stay activation-sized:
wire the minimum RV64 assembler surface and build hooks needed for
`assemble_target_lir_module(...)` and focused wrapper/shared-util proof.
Do not widen immediately into general linker finalization, broad codegen
support, or broad assembler feature auditing.

Keep these families parked:

- broader `emit.cpp` / `RiscvCodegen` support expansion beyond the already
  landed tiny-add native-emitter slice
- RV64 linker activation and executable finalization beyond the first
  assembler-facing binary-utils seam
- the stale `memory.cpp` typed-indirect / broader GEP route
- broader variadic, comparison, calls, and atomics follow-on work
- `inline_asm.cpp`

## Rationale

The branch is still aligned with the accepted RV64 native-emitter /
backend-finalization route. The bounded native-entry seam now exists and both
supported entry points consume the shared peephole finalization surface.

That exhausts the current route. The next honest integration gap is that RV64
still cannot cross the same public binary-utils boundary that x86 and aarch64
already exercise: the backend can emit bounded RV64 assembly text, but the
shared assembly handoff still stops short of object production even though the
translated assembler and linker inventory already exists in-tree.

That makes RV64 assembler activation a more truthful broader continuation than
reopening codegen-owner expansion, revisiting stale parked families, or
jumping straight to linker finalization. The route should first make the
assembler seam real and test-visible; only after that should lifecycle choose
whether a bounded linker-facing follow-on is the next smallest truthful slice.
