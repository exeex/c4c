# Current Packet

Status: Active
Source Idea Path: ideas/open/122_prepared_call_argument_producer_materializability_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Missing Frame-Slot Argument Publication Visibility

## Just Finished

Completed plan Step 4: added shared missing frame-slot call-argument
publication visibility while keeping local aggregate address payload
construction AArch64-local.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

Implemented shared prepared surface:
- Added `PreparedMissingFrameSlotCallArgumentPublicationNeed` and
  `find_prepared_missing_frame_slot_call_argument_publication_need`.
- The query exposes the target-neutral eligibility for missing frame-slot
  synthetic call-argument publication: frame-slot source encoding, source value
  id, GPR single-register ABI destination, prepared source-selection kind, and
  whether the selected source materializes an address.
- The query rejects byval lanes, absent source selections, non-frame-slot
  sources, non-GPR destinations, and multi-register destinations.

AArch64 consumer update:
- `materialize_missing_frame_slot_call_arguments` now consumes the shared
  missing-frame-slot publication need before doing local home lookup,
  emitted-register checks, prepared before-call move selection, memory operand
  construction, register conversion, optional aggregate-address payload
  emission, and machine instruction wrapping.
- Local aggregate address payload construction remains in
  `materialize_local_aggregate_address_payload` and is only described by the
  shared query as `may_emit_local_aggregate_address_payload`.

Visibility proof:
- The prepared printer now emits `missing_frame_slot_arg_publication=yes`,
  the missing-frame-slot publication kind, the source value id, and address
  materialization/local-payload booleans.
- `backend_prepare_frame_stack_call_contract` now proves the missing local
  aggregate frame-slot address argument exposes the shared publication need
  and prepared dump visibility.

## Suggested Next

Execute Step 5: convert any remaining AArch64 calls rediscovery of
target-neutral producer routes to the shared surfaces, then request acceptance
review if no additional consumer work remains.

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
- Missing frame-slot synthetic publication need is now visible through the
  shared call-argument query, but AArch64 still owns emitted-register state,
  ABI register conversion, source memory operand construction, aggregate
  address payload emission, and machine record wrapping.
- Local aggregate address payload construction remains intentionally
  target-local because it creates AArch64 instruction payloads and stack-copy
  address spelling.
- Remaining route gaps: shift/unsigned div/rem/comparison binary producers are
  not classified materializable by the Step 2 shared query.
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
