# Current Packet

Status: Active
Source Idea Path: ideas/open/122_prepared_call_argument_producer_materializability_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert AArch64 Calls To Consume The Shared Surface

## Just Finished

Completed plan Step 5: converted remaining AArch64 call-argument
source-selection consumers in `calls.cpp` to read through the shared prepared
publication-source routing surface while preserving target-local emission.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `todo.md`

AArch64 consumer update:
- `StackFrameSlotCallOperandOwner::sret_memory_return_address_source` now uses
  `find_prepared_call_argument_publication_source_routing` unconditionally
  before falling back to the prepared sret memory-return plan, while preserving
  the old fail-closed behavior when `argument.source_selection` is present but
  the shared routing surface exposes no available source selection.
- `BeforeCallMoveLocalOwner::instruction` now reads frame-slot value/address,
  local-frame-address, prior-preservation, byval-lane, scalar FPR, binary128,
  and stack-destination call-argument source selections through
  `find_prepared_call_argument_publication_source_routing`.
- The existing shared materializability query remains the scalar producer gate
  through `find_prepared_scalar_call_argument_source_producer_materialization`.
- Direct-global select-chain call arguments remain routed through the existing
  shared dependency carried by the call-argument plan, and missing frame-slot
  synthetic publication remains routed through the Step 4 shared need query.

Kept target-local:
- Same-block tracing inside byte/lane, FP, and F128 emission helpers remains
  AArch64-local because those helpers assemble target instruction payloads,
  choose scratch/register views, trace concrete frame bytes, and spell
  selected machine lines. Those paths no longer own the call-argument
  materializability or prepared source-selection decision.

## Suggested Next

Request acceptance review for idea 122. If review agrees, regenerate any
required close proof and hand lifecycle closure to the plan owner.

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
- Remaining `find_same_block_named_producer` use in `calls.cpp` is in
  target-local source emission/tracing helpers rather than call-argument
  shared route selection; do not treat that as materializability authority
  without a separate source idea.
- Present-but-unavailable call-argument source selections must not gain a new
  fallback path while consuming the shared routing query.
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
