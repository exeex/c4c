# 125 Prepared Pending Store-Global Publication Producer Contract

## Goal

Move pending store-global stack-value publication producer selection into the
shared prepared publication contract so AArch64 memory lowering consumes a
prepared producer instruction/index fact instead of rediscovering the producer
by scanning BIR instructions.

## Why This Exists

The AArch64 memory boundary audit found one remaining shared-authority gap in
`lower_pending_store_global_stack_value_publications`. After calling
`prepare::plan_pending_prepared_store_global_publications`, the AArch64 helper
still scans earlier `context.bir_block->insts`, calls
`instruction_result_value`, and matches `result->name ==
plan.source_value.name` plus type to recover the stack-value publication
producer before emission.

That repeats target-neutral producer identity work in the target backend. It is
distinct from closed store-source and dump-visibility routes because the
prepared pending store-global candidate already exists, but it does not yet
carry the producer instruction/index needed by the AArch64 consumer.

## Owner Boundary

Shared prepared/prealloc should own the decision that a pending store-global
publication has a specific source producer instruction/index, including
fail-closed behavior when no unique producer can be identified.

AArch64 memory lowering should own only the target-local emission from that
prepared fact: checking publication availability, selecting/registering stack
publication state, calling the existing publication emitter with the prepared
producer instruction, and spelling the resulting AArch64 machine instruction.

## Likely Files

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## In Scope

- Extend `PreparedStoreGlobalPublicationCandidate` or an adjacent prepared
  publication record so pending store-global candidates expose the resolved
  source producer instruction/index when available.
- Teach `plan_pending_prepared_store_global_publications` to populate that
  fact from shared prepared/BIR inputs and to fail closed for ambiguous or
  missing producers.
- Update `lower_pending_store_global_stack_value_publications` to consume the
  prepared producer fact instead of scanning earlier BIR instructions by result
  name and type.
- Add or update tests proving both the shared prepared fact and the AArch64
  consumer behavior.

## Out Of Scope

- Reworking general store-source publication planning already covered by ideas
  34, 39a, and 111.
- Reopening the broader AArch64 memory prepared-authority cleanup from idea 50.
- Moving AArch64 opcode choice, scratch policy, machine-record construction, or
  memory operand emission into shared prepared code.
- Creating an AArch64 memory-private physical split; that is local-clarity work
  and should wait until this shared-authority gap is resolved or explicitly
  scoped out.

## Proof Route

- Add shared-prepared coverage in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp` showing a
  pending store-global candidate carries the expected producer instruction/index
  and fails closed when the producer is absent or ambiguous.
- Update AArch64 consumer coverage in
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  so the pending publication path still emits the stack-homed publication using
  the prepared producer fact.
- Run the focused backend test subset selected by the supervisor, with at
  least the two test files above included.

## Acceptance And Completion Criteria

- `lower_pending_store_global_stack_value_publications` no longer scans
  `context.bir_block->insts` to rediscover a producer by result name/type.
- The pending store-global prepared publication result carries enough producer
  information for AArch64 to emit or fail closed without local producer
  recovery.
- Existing duplicate-publication, stack-home-only, and pending-publication
  behavior is preserved.
- Test evidence covers both the prepared contract and the AArch64 consumer.

## Reviewer Reject Signals

- The patch keeps or recreates the local AArch64 scan over earlier BIR
  instructions for `result->name == plan.source_value.name` plus type.
- The patch replaces the scan with another name-shaped shortcut, testcase-only
  producer match, or hard-coded instruction offset instead of a prepared
  producer contract.
- The patch claims progress mainly through expectation rewrites, weaker
  unsupported contracts, helper renames, or classification-only changes.
- The patch broadens into unrelated store-source planning, dispatch/call
  publication, memory-owner physical splitting, or AArch64 instruction
  emission rewrites.
- The same producer-rediscovery failure mode remains hidden behind a new helper
  name in `memory.cpp`.

## Closure Evidence

Closed on 2026-06-08.

Pending store-global candidates now carry prepared source producer authority:
the shared pending publication plan records producer kind, block label, and
instruction index for valid prepared store-source plans. Shared planning rejects
missing, ambiguous, and no-authority producer cases before publishing
candidates, including the reviewer follow-up that removed the public default
null-authority path.

AArch64 pending store-global publication lowering now consumes the prepared
producer instruction index instead of scanning earlier BIR instructions by
source name and type. It keeps target-local responsibility for availability
checks, stack publication state, publication emission, and machine instruction
spelling.

Evidence commits:

- `0772e0b6a` added shared prepared producer facts for pending store-global
  publication candidates.
- `92dd73bb1` rewired the AArch64 consumer to use the prepared producer fact.
- `14588a9a4` recorded reviewer follow-up for the remaining no-authority
  shared-planner contract leak.
- `c8b0809cd` tightened shared planning to fail closed without producer lookup
  authority.

Reviewer report `review/idea125_code_review.md` judged the implementation route
on track; the medium watch item was addressed by `c8b0809cd`.

Proof:

- Backend proof after each code packet used
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
  and was accepted into the canonical baseline.
- Close-time backend proof regenerated `test_after.log` with the same command:
  `100% tests passed, 0 tests failed out of 179`.
- Close-time regression guard compared `test_before.log` and `test_after.log`
  with `--allow-non-decreasing-passed`: before 179 passed / 0 failed, after 179
  passed / 0 failed, no new failing tests, result `PASS`.

Completion criteria satisfied:

- `lower_pending_store_global_stack_value_publications` no longer rediscovers
  pending producers by local name/type scan.
- The prepared pending store-global result carries enough producer information
  for AArch64 emission or shared fail-closed rejection.
- Duplicate-publication, stack-home-only, and pending-publication behavior is
  preserved.
- Tests cover shared prepared producer publication, missing/ambiguous/no-
  authority fail-closed behavior, and AArch64 prepared consumer behavior.
