# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Selected Producer-Chain Consumer

## Just Finished

Completed Step 4 by migrating the AArch64 scalar call-argument binary producer
materialization consumer to `PreparedCallArgumentBinaryProducerMaterializationFact`
plus `verify_prepared_call_argument_binary_producer_materialization_contract`.

Route 6 binary producer evidence now only identifies the candidate path; the
binary materialization family is admitted only when the prepared typed fact is
present and verifies coherent. Missing or malformed prepared binary facts fail
closed without target-local binary expression recovery, while existing non-binary
producer handling remains unchanged.

Focused AArch64 dispatch coverage now proves coherent prepared binary
materialization still emits the scalar add, Route 6-only binary materialization
is rejected without prepared typed facts, and an incoherent prepared binary
producer payload does not recover through the target consumer.

## Suggested Next

Begin Step 5 by running broader validation and recording whether the source idea
can close or needs another follow-up runbook for remaining materialization
families.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- `find_prepared_call_argument_binary_producer_materialization_fact` currently
  filters malformed prepared binary records before returning a typed fact; the
  AArch64 consumer observes those as missing facts and still verifies the
  `nullptr` fact before failing closed.
- Route 6-only duplicate/ambiguous facts remain target-side evidence and still
  must not be treated as prepared fact coherence.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
