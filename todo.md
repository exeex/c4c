# Current Packet

Status: Active
Source Idea Path: ideas/open/122_prepared_call_argument_producer_materializability_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Shared Producer Materializability Fact Or Query

## Just Finished

Completed plan Step 2: added shared prepared call-argument source-producer
materializability query/fact coverage and wired AArch64 to consume it.

Changed files:
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

Implemented shared prepared surface:
- Added `PreparedCallArgumentSourceProducerMaterialization`.
- Added `prepared_call_argument_binary_producer_opcode_is_materializable`.
- Added `find_prepared_call_argument_source_producer_materialization`.
- The query reuses prepared same-block source-producer identity, block,
  instruction order, and result-name/type checks; it does not encode AArch64
  register spelling, scratch selection, ABI register policy, or machine
  instruction wrapping.
- Materializable ordinary scalar routes are currently `LoadLocal` plus the
  existing ordinary binary subset: add/sub/and/or/xor/mul/sdiv/srem.

AArch64 consumer update:
- `dispatch_lookup.cpp` now delegates the old call-argument binary opcode
  predicate to the shared prepared predicate.
- `find_same_block_scalar_producer` consumes the shared materialization query
  while preserving the existing prepared-lookup fallback path.
- `calls.cpp` consumes the shared query before recursively emitting prepared
  scalar call-argument producers; recursive operand lowering, register
  retargeting, emitted-register recording, and machine instruction emission
  remain AArch64-local.

Visibility proof:
- `backend_prepare_frame_stack_call_contract` now has
  `check_call_argument_source_producer_materializability_contract`, which
  verifies shared query visibility for a load-local producer and an ordinary
  binary call-argument producer, and verifies fail-closed behavior for a
  non-covered binary producer and a future producer.

## Suggested Next

Execute Step 3: classify remaining call-argument producer gaps now that the
shared materializability query exists. Recommended focus: decide whether
publication-source routing should be folded into this shared surface or remain
a separate prepared query, and separately classify direct-global select-chain,
missing frame-slot publication need, and local aggregate address payloads.

## Watchouts

- Do not reopen idea 116 producer, current-block publication, join-routing, or
  select-chain authority.
- Do not add AArch64 same-block producer scans, BIR-name matching, or
  direct-global/select-chain named-case shortcuts.
- Do not move AArch64 scalar instruction spelling, register retargeting,
  select-chain assembler lines, local aggregate address payloads, or machine
  instruction wrapping into shared code.
- Remaining route gaps: direct-global select-chain still has a separate
  prepared dependency query; missing frame-slot synthetic publication need is
  still embedded in AArch64 iteration/filtering; local aggregate address
  payload construction remains intentionally target-local; shift/unsigned
  div/rem/comparison binary producers are not classified materializable by this
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
