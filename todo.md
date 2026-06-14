Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Authority Bucket Classification

# Current Packet

## Just Finished

Step 2 - Authority Bucket Classification classified the Step 1
`PreparedBirModule::liveness` inventory by the authority each consumer
exercises.

Candidate identity-only rows:

- `tests/backend/bir/backend_prepare_liveness_test.cpp` direct checks of
  value IDs, block labels, CFG edge IDs, and function membership are
  identity-only metadata when they assert that liveness records name-table
  identities without using those facts for allocation or target policy.
- `src/backend/backend.cpp::filter_prepared_module_to_function(...)` is
  status/debug compatibility plus identity-only filtering. It uses function
  identity to erase or retain `filtered.liveness.functions`, and it keeps
  liveness aligned with other filtered prepared aggregates. It does not by
  itself allocate registers or choose storage, but it blocks deletion or
  private pass-context movement unless the prepared dump/filtering surface
  keeps an equivalent retained liveness compatibility record.
- `src/backend/prealloc/liveness.cpp::BirPreAlloc::run_liveness()` has a
  construction-only role for clearing and repopulating
  `prepared_.liveness.functions`. The records it constructs are not
  identity-only overall because their intervals, call points, loop depth, and
  cross-call facts feed policy consumers below.

Blocked allocation, target-register, move-scheduling, and storage rows:

- `src/backend/prealloc/regalloc.cpp::BirPreAlloc::run_regalloc()` is the
  strongest demotion blocker. It combines liveness identity with allocation
  and target-register policy by copying value names/types/kinds,
  `live_interval`, and `crosses_call` into `PreparedRegallocValue`, selecting
  register classes/group widths, choosing caller-saved versus callee-saved
  preferences, building interference from overlapping intervals, assigning
  registers, falling back to stack slots, and publishing `value_locations`.
- `src/backend/prealloc/regalloc/intervals.cpp` is allocation policy. It uses
  liveness intervals, loop depth, use points, and `crosses_call` for value
  priority, weighted use scoring, interval overlap, and start-point ordering.
- `src/backend/prealloc/regalloc/classification.cpp` is target-register
  policy. It combines `PreparedLivenessValue` identity/type with module
  register overrides to resolve register class and group width.
- `src/backend/prealloc/regalloc/spill_reload.cpp` is storage/home and move
  scheduling. It consumes `PreparedLivenessFunction` block/use-point facts to
  place spill/reload operations.
- `src/backend/prealloc/regalloc/{phi_moves.cpp,consumer_moves.cpp,call_moves.cpp,move_records.cpp}`
  are move-scheduling and target-storage consumers through the regalloc/value
  location records that were seeded from liveness. They do not directly read
  `PreparedBirModule::liveness`, but demoting liveness cannot disturb their
  liveness-derived move bundle timing, storage kind, register placement, or
  stack offset facts.

Blocked call and helper-planning rows:

- `src/backend/prealloc/call_plans.cpp::populate_call_plans(...)` is
  carrier/helper plus storage/home plus target-storage policy. Its local
  `find_liveness_function(...)` and liveness-to-program-point mapping feed
  `build_call_preserved_values(...)`, preservation routes, source/destination
  endpoints, callee-saved register versus stack-slot decisions, and call
  boundary effects. This combines identity with policy and blocks demotion,
  deletion, wrapping, and private-pass-context movement.
- `src/backend/prealloc/i128_runtime_helpers.cpp` is carrier/helper,
  target-register, target-storage, and status compatibility authority. Its
  local liveness lookup and call-point helpers combine helper identity with
  regalloc/value-location facts to build live-preservation policies,
  preserved-value routes, selected call ownership, and missing-fact rows such
  as `live_preservation_requires_structured_live_across_helper_facts` and
  `live_preservation_requires_complete_preserved_value_routes`.
- `src/backend/prealloc/f128_runtime_helpers.cpp` has the same carrier/helper,
  target-register, target-storage, and status compatibility role for f128
  helpers. It also gates selected call ownership on live-preservation policy,
  so it blocks any movement that hides liveness from helper planning.

Blocked output, target, and compatibility rows:

- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp` is output policy
  and status/debug compatibility. It was not found to read `prepared.liveness`
  directly, but it prints liveness-derived `live_preservation=[...]`,
  preserved-value counts, and selected-call ownership fields. Printer output is
  therefore blocked on retained helper live-preservation semantics, not a
  one-reader liveness demotion candidate.
- AArch64 MIR code has no direct `PreparedBirModule::liveness` read in the
  Step 1 scan, but it consumes liveness-derived regalloc, value homes, call
  plans, preserved-value routes, and i128/f128 live-preservation policy. This
  is target-register, target-storage, carrier/helper, and output/status
  compatibility authority.
- x86 MIR code has no direct `PreparedBirModule::liveness` read in the Step 1
  scan, but it consumes liveness-derived `PreparedRegalloc`,
  `value_locations`, and `call_plans` through prepared aggregate/query
  wrappers. This is derived target-register, target-storage, storage/home, and
  debug compatibility authority.
- RISC-V MIR code has no concrete direct `liveness`, `live_interval`, or
  `live_preservation` read in the Step 1 scan. It remains an aggregate input
  and derived-value-location risk only: a compatibility blocker for the
  prepared aggregate, but not current proof of a direct liveness reader.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`,
  `tests/backend/bir/backend_prepared_printer_test.cpp`, and liveness tests
  that assert regalloc projection, call points, cross-call flags,
  helper-family preservation, or printed live-preservation rows are
  status/debug compatibility plus behavioral proof rows. They block weakening
  or hiding liveness unless equivalent public prepared behavior is proven.

Consumers that combine identity with policy/allocation decisions:

- `BirPreAlloc::run_regalloc()` combines value identity with live intervals,
  cross-call flags, register-class/group-width policy, interference, physical
  register assignment, stack-slot fallback, and value-location publication.
- `regalloc_detail::value_priority(...)`,
  `weighted_use_score(...)`, `intervals_overlap(...)`, and
  `append_spill_reload_ops(...)` combine identity with allocation priority,
  interval conflict, and spill/reload placement.
- `populate_call_plans(...)` combines function/value identity with call
  program points, value homes, register/stack endpoints, and preservation
  routes.
- i128/f128 runtime-helper planners combine helper/callee identity with call
  program points, preserved-value routes, target register/storage facts, and
  selected call ownership.

Named demotion blockers:

- Demotion or deletion is blocked by `BirPreAlloc::run_regalloc()`,
  regalloc interval/classification/spill-reload helpers,
  `populate_call_plans(...)`, i128/f128 runtime-helper planners, and tests that
  assert the public liveness/regalloc/helper contract.
- Wrapping is blocked unless the wrapper preserves all current direct
  liveness readers plus derived regalloc, call-plan, helper, printer, and
  target compatibility surfaces with fail-closed behavior.
- Private-pass-context movement is blocked because prepared module consumers
  outside the construction pass still read `prepared.liveness` directly
  (`call_plans`, i128/f128 helpers, filtering, tests) or consume public
  liveness-derived prepared facts (printer, AArch64, x86, RISC-V aggregate
  boundary).

Step 1 unresolved input decisions:

- Duplicated local `find_liveness_function(...)` helpers are repeat
  implementations of the same liveness-to-function lookup shape, but they are
  not one authority. Each call site uses the lookup for different policy:
  call preservation, i128 helper live preservation, f128 helper live
  preservation, and tests. Preserve them as separate blockers until a later
  idea proves a shared lookup/accessor without changing authority.
- `PrepareOptions::run_liveness` remains ambiguous. Tests call
  `run_liveness()` manually and toggle the option, while the normal
  `BirPreAlloc::run()` pipeline currently calls `run_liveness()`
  unconditionally before regalloc. Step 3 must treat absent/skipped liveness as
  a fail-closed row, not as evidence that the field is optional.
- Derived target/printer boundaries stay distinct: printer is output/status
  compatibility; AArch64 and x86 are derived target-register/target-storage
  and helper policy consumers; RISC-V is currently an aggregate compatibility
  risk with no direct liveness-derived reader identified.

## Suggested Next

Execute Step 3 by building the fail-closed behavior matrix for candidate and
ambiguous rows: absent, invalid, stale, mismatch, fallback,
duplicate/conflict, unsupported, and policy-sensitive liveness states.

## Watchouts

- This is analysis-only; do not edit implementation code or expectations under
  this active plan.
- Candidate identity-only rows are narrow and compatibility-bound. Filtering
  and construction-only rows are not enough for a demotion candidate while
  regalloc, call-plan, and helper-planning readers remain public consumers.
- Step 3 should treat `PrepareOptions::run_liveness` as unresolved normal-path
  ambiguity and require fail-closed behavior for absent or skipped liveness.
- Direct-field consumers are fewer than derived consumers. Do not collapse
  printer, AArch64, x86, or RISC-V into "no liveness risk" because prepared
  output and target behavior depend on liveness-derived facts.

## Proof

`git diff --check -- todo.md`

Passed for this packet. The delegated proof command does not produce
`test_after.log`; no replacement log was created.
