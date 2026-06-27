# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Typed Producer-Chain Materialization Fact

## Just Finished

Completed Step 2 by adding
`PreparedCallArgumentBinaryProducerMaterializationFact` and
`find_prepared_call_argument_binary_producer_materialization_fact` for the
selected AArch64 scalar call-argument same-block binary producer family.

The typed query exposes coherent prepared facts only: destination value name and
type, producer block/instruction identity, producer kind `Binary`, binary
opcode, operands, and same-block-before-call scheduling authority. The existing
compatibility query for call-argument producer materialization now routes binary
admission through the typed fact while preserving the existing load-local path.

Focused coverage was added for coherent facts plus rejected missing lookup,
cross-family load payload, unsupported opcode, mismatched destination type,
future producer, and stale producer payload cases. Existing AArch64 dispatch
coverage continues to cover Route 6 ambiguity/rejection and prepared fallback
behavior without broadening target recovery.

## Suggested Next

Begin Step 3 by adding producer-side verifier status/report coverage for the
typed binary call-argument producer materialization fact, mapping missing facts
to `producer_missing` and contradictory/cross-family facts to
`producer_incoherent`.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- `PreparedCallArgumentBinaryProducerMaterializationFact` is currently a typed
  compatibility view over existing prepared same-block source-producer records;
  Step 3 should add verifier classification before Step 4 migrates the final
  AArch64 consumer contract.
- Route 6-only duplicate/ambiguous facts remain target-side evidence and should
  not be treated as prepared fact coherence without prepared fallback.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
