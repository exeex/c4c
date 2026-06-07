# Current Packet

Status: Active
Source Idea Path: ideas/open/122_prepared_call_argument_producer_materializability_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Publication-Source And Select-Chain Dependency Visibility

## Just Finished

Completed plan Step 3: added shared call-argument publication-source routing
visibility and direct-global select-chain dependency visibility through the
prepared call-argument surface without reopening idea 116 authority.

Changed files:
- `src/backend/prealloc/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/select_materialization.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

Implemented shared prepared surface:
- Added `PreparedCallArgumentPublicationSourceRouting` and
  `find_prepared_call_argument_publication_source_routing`.
- The query exposes target-neutral call-argument source encoding, source value
  and computed-address routing fields, prepared source-selection routing, and
  the existing direct-global select-chain dependency pointer.
- The query consumes existing idea 116 direct-global/select-chain dependency
  facts through `PreparedCallArgumentDirectGlobalSelectChainDependency`; it
  does not rescan producers, current blocks, joins, or select chains.

AArch64 consumer update:
- `calls.cpp` now reads call-argument source-selection routing through the
  shared publication-source query for sret/frame-slot, before-call move, and
  local aggregate address paths.
- `select_materialization.cpp` now reads direct-global select-chain dependency
  through the shared publication-source query.
- Register selection, scratch policy, select-chain assembler construction,
  aggregate address payload construction, and machine instruction wrapping
  remain AArch64-local.

Visibility proof:
- `backend_prepare_frame_stack_call_contract` now has
  shared-query checks for symbol-address and computed-address call-argument
  publication-source routing.
- The direct-global select-chain call-argument contract now proves dependency
  visibility through the shared routing query and the prepared dump.

## Suggested Next

Execute Step 4: add or classify missing frame-slot argument publication
visibility. Recommended focus: identify the target-neutral missing frame-slot
publication need that still drives `materialize_missing_frame_slot_call_arguments`,
keep local aggregate address payload construction AArch64-local, and prove the
neighboring aggregate-address route remains covered.

## Watchouts

- Do not reopen idea 116 producer, current-block publication, join-routing, or
  select-chain authority.
- Do not add AArch64 same-block producer scans, BIR-name matching, or
  direct-global/select-chain named-case shortcuts.
- Do not move AArch64 scalar instruction spelling, register retargeting,
  select-chain assembler lines, local aggregate address payloads, or machine
  instruction wrapping into shared code.
- Direct-global select-chain dependency is now visible through the shared
  call-argument publication-source routing query, but the existing dependency
  authority remains the idea 116 call-plan fact.
- Remaining route gaps: missing frame-slot synthetic publication need is still
  embedded in AArch64 iteration/filtering; local aggregate address payload
  construction remains intentionally target-local; shift/unsigned
  div/rem/comparison binary producers are not classified materializable by the
  Step 2 shared query.
- Treat expectation downgrades, unsupported-path rewrites, and widened target
  implementation edits as route failures.

## Proof

Ran exact delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_prepared_memory_operand_records)$' >> test_after.log 2>&1`

Result: passed, 5/5 tests.
Log path: `test_after.log`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 5/5 before and 5/5 after, no new failures.
