# RV64 Prepared Emitter Context Interface Cleanup Runbook

Status: Active
Source Idea: ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md

## Purpose

Refine the interfaces created by the RV64 prepared emitter decomposition so
shared prepared lookup state flows through a small owner-local support surface
instead of broad helper headers and repeated raw parameter bundles.

Goal: make prepared RV64 owner boundaries easier to extend without changing
emitted behavior or adding new backend capability.

## Core Rule

This is a behavior-preserving interface cleanup. Do not add new RV64 runtime
cases, weaken unsupported behavior, or repair unrelated backend capability
under this runbook.

## Read First

- Source idea:
  `ideas/open/302_rv64_prepared_emitter_context_interface_cleanup.md`
- Current prepared RV64 owner files:
  `src/backend/mir/riscv/codegen/prepared_*_emit.{hpp,cpp}`
- Public compatibility and legacy direct-LIR glue:
  `src/backend/mir/riscv/codegen/emit.cpp`
- Build metadata for new codegen files, if a support file is added.

## Current Targets

- `prepared_scalar_emit.{hpp,cpp}`
- `prepared_local_memory_emit.{hpp,cpp}`
- `prepared_frame_emit.{hpp,cpp}`
- `prepared_call_emit.{hpp,cpp}`
- `prepared_function_emit.{hpp,cpp}`
- new support files such as `prepared_emit_context.{hpp,cpp}` if they stay
  small and lookup-oriented

## Non-Goals

- Do not add globals, indirect calls, varargs, stack arguments, aggregate ABI,
  dynamic stack, object emission, or any other new RV64 capability.
- Do not rewrite RV64 around another backend architecture.
- Do not change the qemu runtime test-runner contract.
- Do not fix the known `backend_riscv_prepared_edge_publication` baseline
  failure unless a separate idea owns that repair.
- Do not hide unsupported behavior through test or expectation downgrades.
- Do not let a new support file become an unbounded miscellaneous helper pile.

## Working Model

- Owner files remain organized by backend responsibility: scalar, local memory,
  frame, call, edge publication, function, and module emission.
- Shared prepared lookup operations belong in a narrow support/context surface
  when multiple owners need them.
- Feature-family headers should expose feature emission APIs, not every shared
  lookup helper.
- Current-location data such as block label and instruction index can move into
  a small context only when that reduces repeated bundles without hiding
  ownership.
- Frame and stack-slot helpers should stay under frame/support ownership unless
  a feature owner has a clear local reason to expose them.

## Execution Rules

- Prefer small mechanical moves with fresh build or test proof after each
  meaningful extraction.
- Preserve emitted assembly behavior and fail-closed handling for unsupported
  RV64 forms.
- Keep `emit.cpp` thin; do not move prepared lowering ownership back into it.
- Update build metadata with any new support source file in the same step that
  introduces the file.
- If a helper needs semantic redesign instead of interface movement, stop and
  open a separate idea rather than changing behavior here.
- Treat testcase-shaped shortcuts, expectation churn, and BIR/LLVM fallback as
  route failures.

## Steps

### Step 1. Inventory Shared Prepared Lookup Surfaces

Goal: separate true owner APIs from shared prepared lookup utilities before
moving code.

Primary target: headers under `src/backend/mir/riscv/codegen/prepared_*_emit.hpp`.

Actions:

- Inspect exported helpers in scalar, local-memory, frame, call, function, edge
  publication, and module prepared emit headers.
- Classify helpers into owner emission APIs, shared prepared lookup utilities,
  frame/support utilities, and local implementation details.
- Identify repeated parameter bundles involving `PreparedNameTables`,
  `PreparedFunctionLookups*`, `BlockLabelId`, and instruction indices.
- Record the first extraction target in `todo.md` before changing code.

Completion check:

- `todo.md` names the selected first extraction target and the proof command the
  executor should run for that packet.
- No implementation files are changed in this step unless the executor is
  explicitly delegated an implementation packet.

### Step 2. Introduce A Narrow Prepared Emit Context

Goal: add a small support header/source pair for shared prepared lookup state
without changing behavior.

Primary target: a new support surface such as
`src/backend/mir/riscv/codegen/prepared_emit_context.{hpp,cpp}`.

Actions:

- Define the smallest useful context or lookup handle around prepared names,
  function lookups, and optional current-location metadata.
- Keep construction explicit at owner boundaries; do not hide which function,
  block, or instruction is being emitted.
- Move only lookup/support declarations justified by at least two owners or by
  clear owner-boundary cleanup.
- Add the new source to build metadata when needed.

Completion check:

- Existing prepared emit code builds with the new support files present.
- The support API is narrow enough that scalar/call/memory feature lowering
  would not naturally accumulate there.
- Run at least build proof and the supervisor-selected RV64 runtime subset.

### Step 3. Move Value-Home And Register Lookup Helpers

Goal: remove shared prepared value/register queries from the scalar feature
header.

Primary target: `prepared_scalar_emit.{hpp,cpp}` plus the support/context
surface.

Actions:

- Move shared helpers such as `prepared_value_home_for`,
  `prepared_register_for_value`, `prepared_pointer_register_for_value`,
  `prepared_register_for_value_name_id`, and rematerializable immediate queries
  into the support surface when they are not scalar-specific.
- Update scalar, local-memory, call, edge-publication, and function owners to
  include the support header instead of depending on scalar for shared lookup
  utilities.
- Preserve helper behavior and existing unsupported paths.

Completion check:

- Scalar headers stop acting as the catch-all prepared lookup API.
- Cross-owner call sites compile through the support/context API.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes or reports only a documented pre-existing failure.

### Step 4. Reduce Current-Location Parameter Bundles

Goal: replace repeated raw prepared tables plus block/instruction coordinates
with a clearer context where it reduces noise.

Primary target: scalar, local-memory, call, and function prepared emission
call sites.

Actions:

- Identify call paths repeatedly passing `PreparedNameTables`,
  `PreparedFunctionLookups*`, `BlockLabelId`, and instruction index.
- Replace repeated bundles with the context or narrow lookup handle from Step 2
  when it makes ownership clearer.
- Keep function/block/instruction information visible at boundaries that need
  it for readability or correctness.

Completion check:

- Repeated cross-owner signatures are shorter and clearer.
- The diff does not obscure which block or instruction a lookup refers to.
- Run RV64 runtime validation after the move.

### Step 5. Clean Frame And Local-Memory Support Boundaries

Goal: keep stack/frame-slot helpers under frame/support ownership and reduce
scalar/local-memory leakage.

Primary target: `prepared_frame_emit.*`,
`prepared_local_memory_emit.*`, and affected scalar users.

Actions:

- Revisit helpers such as `simple_frame_slot_sp_offset_for`,
  `emit_i32_load_from_stack_offset`, and `emit_i32_store_to_stack_offset`.
- Move or expose them through the owner that best matches their responsibility.
- Break avoidable scalar-to-local-memory or local-memory-to-scalar include
  coupling without changing generated assembly.

Completion check:

- Stack load/store and frame-slot offset utilities have clear ownership.
- Scalar and local-memory headers expose less shared infrastructure than the
  starting layout.
- Run focused RISC-V/backend route validation selected by the supervisor.

### Step 6. Final Interface Review And Closure Proof

Goal: confirm the cleanup achieved the source idea without drifting into
capability work.

Primary target: all prepared RV64 codegen headers and build metadata touched by
this runbook.

Actions:

- Review public prepared emit headers for remaining owner leakage.
- Confirm `emit.cpp` remains thin and does not regain prepared lowering
  ownership.
- Confirm unsupported RV64 forms remain fail-closed.
- Escalate any discovered semantic redesign need into a separate idea instead
  of implementing it here.

Completion check:

- Acceptance criteria in the source idea are satisfied.
- Run:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- Before closure, run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Any remaining failure is explicitly documented as an accepted baseline rather
  than introduced by this cleanup.
