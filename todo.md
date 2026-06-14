Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Liveness Consumer Inventory

# Current Packet

## Just Finished

Step 1 - Liveness Consumer Inventory recorded the current
`PreparedBirModule::liveness` producer/consumer surface.

Producer inventory:

- `src/backend/prealloc/module.hpp`: `PreparedBirModule` owns the public
  `PreparedLiveness liveness` aggregate field. `liveness.hpp` defines
  `PreparedLivenessFunction`, `PreparedLivenessBlock`,
  `PreparedLivenessValue`, `PreparedLiveInterval`, `call_points`, `live_in`,
  `live_out`, `block_loop_depth`, `definition_point`, `use_points`,
  `crosses_call`, and per-value intervals.
- `src/backend/prealloc/prealloc.cpp`: `BirPreAlloc::run()` calls
  `run_liveness()` after stack layout and before out-of-SSA/regalloc, so
  liveness is upstream of allocation, value-location, call, storage, and
  helper facts.
- `src/backend/prealloc/liveness.cpp`: `BirPreAlloc::run_liveness()` clears
  `prepared_.liveness.functions`, reserves one record per defined BIR
  function, computes dense values, block IDs, successors/predecessors,
  phi-predecessor uses, program points, live-in/live-out dataflow,
  live-through interval extension, call points, loop depth, per-value
  intervals, and pushes each `PreparedLivenessFunction`. It also records the
  `liveness` completed phase/note.
- `src/backend/prealloc/names.hpp`: `PrepareOptions::run_liveness` exists and
  tests toggle it, but `BirPreAlloc::run()` currently calls `run_liveness()`
  unconditionally in the normal pipeline.

Direct consumer inventory:

- Regalloc and value-location producer:
  `src/backend/prealloc/regalloc.cpp::BirPreAlloc::run_regalloc()` reserves
  `prepared_.regalloc.functions` and `prepared_.value_locations.functions`
  from `prepared_.liveness.functions`, walks each `PreparedLivenessFunction`,
  copies value identity/type/kind/interval/cross-call facts into
  `PreparedRegallocValue`, resolves register class/group width, builds
  allocation constraints, target saved-vs-caller-saved preferences,
  interference edges from overlapping intervals, allocation order, register
  assignments, stack slot fallback, spill points, move resolution, helper
  mappings, and final `value_locations`.
- Regalloc derived helpers:
  `src/backend/prealloc/regalloc/intervals.cpp` consumes
  `PreparedLivenessValue` and `PreparedLivenessFunction` for priority,
  loop-depth-weighted use scoring, interval overlap, and program-point
  location. `src/backend/prealloc/regalloc/spill_reload.cpp` consumes
  `PreparedLivenessFunction` blocks/use points to place spill/reload ops.
  `src/backend/prealloc/regalloc/classification.cpp` consumes
  `PreparedLivenessValue` plus module register overrides to choose target
  register class/group width.
- Call-plan producer:
  `src/backend/prealloc/call_plans.cpp` has a local
  `find_liveness_function(const PreparedLiveness&, FunctionNameId)` and
  reads `prepared.liveness` in `populate_call_plans(...)`. It maps liveness
  blocks to call program points and combines live intervals, regalloc homes,
  frame plans, and call-preservation candidates to populate
  `PreparedCallPlan::preserved_values` and call preservation endpoints/routes.
- Runtime helper planners:
  `src/backend/prealloc/i128_runtime_helpers.cpp` and
  `src/backend/prealloc/f128_runtime_helpers.cpp` each define local liveness
  lookup/program-point helpers, read `prepared.liveness`, and use liveness
  plus regalloc/value locations to compute helper live-preservation policies,
  preserved values, selected call ownership, and fail-closed missing facts such
  as `live_preservation_requires_structured_live_across_helper_facts` and
  `live_preservation_requires_complete_preserved_value_routes`.
- CLI/debug filtering:
  `src/backend/backend.cpp::filter_prepared_module_to_function(...)` clears or
  erases `filtered.liveness.functions` alongside control-flow, value-location,
  addressing, regalloc, stack-layout, and liveness notes when a prepared dump is
  filtered to one function.
- Tests/direct assertions:
  `tests/backend/bir/backend_prepare_liveness_test.cpp` defines
  `find_liveness_function(...)` over `prepared.liveness.functions` and asserts
  liveness values, intervals, call points, block labels/edges, loop depth,
  cross-call flags, regalloc projection from liveness, and helper-family
  liveness contracts. `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  similarly reads `prepared.liveness.functions` to assert call points for
  call-frame/dynamic-stack contracts.

Derived and output consumer inventory:

- Prepared printer/status output:
  No direct `prepared.liveness` printer read was found. The prepared printer
  does print liveness-derived helper policy/status fields in
  `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`, including
  `live_preservation=[...]`, preserved-value counts, and selected call
  ownership `live_preservation=yes/no` for i128/f128 helpers. Printer tests in
  `tests/backend/bir/backend_prepared_printer_test.cpp` assert those derived
  rows and missing-fact behavior.
- AArch64 backend:
  No direct `PreparedBirModule::liveness` read was found under
  `src/backend/mir/aarch64`. AArch64 lowering consumes liveness-derived
  prepared facts: regalloc/value homes, call plans, preserved-value routes,
  and i128/f128 live-preservation policies. Tests under
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `backend_aarch64_target_instruction_records_test.cpp`, and
  `backend_aarch64_machine_printer_test.cpp` mutate/assert helper
  `live_preservation_policy` and selected call ownership as fail-closed target
  behavior.
- x86 backend:
  No direct `PreparedBirModule::liveness` read was found under
  `src/backend/mir/x86`. x86 consumes liveness-derived `PreparedRegalloc`,
  `value_locations`, and `call_plans` through prepared aggregate/query wrappers
  such as `src/backend/mir/x86/x86.hpp`, `prepared/prepared.hpp`,
  `module/module.cpp`, and `debug/debug.cpp`.
- RISC-V backend:
  No direct `PreparedBirModule::liveness`, `live_interval`, or
  `live_preservation` read was found under `src/backend/mir/riscv`. RISC-V
  still accepts the prepared aggregate in `src/backend/mir/riscv/codegen/emit.cpp`,
  so Step 2 should treat it as an aggregate/wrapper risk only unless a concrete
  liveness-derived field consumer is identified.

Unresolved Step 2 classification inputs:

- Whether duplicated local `find_liveness_function(...)` helpers in call
  plans and i128/f128 helper planners are separate ownership authorities or
  repeat implementations of one liveness-to-call-point query.
- Whether `PrepareOptions::run_liveness` is intentionally ignored by the
  normal `BirPreAlloc::run()` path or is legacy scaffolding used only by tests
  that call pass methods manually.
- Which derived target rows should be classified as allocation/storage/helper
  blockers versus status/debug compatibility rows: regalloc/value locations,
  call preservation, runtime helper live preservation, prepared-printer helper
  rows, AArch64 target instruction records, x86 wrappers/debug, and RISC-V
  aggregate input should stay distinct in Step 2.

## Suggested Next

Execute Step 2 by classifying the inventory above into identity-only metadata,
allocation, move scheduling, storage/home, carrier/helper, target register,
target storage, output policy, status/debug compatibility, and
construction-only buckets.

## Watchouts

- This is analysis-only; do not edit implementation code or expectations under
  this active plan.
- Do not treat the completed route/invariants/completed_phases/notes metadata
  gate as evidence that `liveness` is demotion-ready.
- Mark allocation, storage, helper-planning, target-register,
  target-storage, move-scheduling, and output-policy consumers as blockers
  unless a later source idea proves a narrower semantic handoff.
- Direct-field consumers are fewer than derived consumers. Step 2 must not
  collapse derived AArch64/x86/RISC-V/printer behavior into "no direct
  liveness read" because regalloc, call plans, and runtime-helper policies are
  liveness-derived authority.

## Proof

`git diff --check -- todo.md`
