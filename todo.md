# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Producer-Chain Boundaries

## Just Finished

Mapped current same-block producer-chain boundaries and selected the first
bounded producer-chain family.

Primary producer-chain surfaces:

- RV64 `prepared_scalar_emit.cpp` still has local raw-BIR helpers such as
  `find_same_block_select_producer` and `find_immediate_compare_producer`.
  These are clear target-recovery candidates, but they do not yet expose a
  compact prepared producer fact surface for a narrow first typed query.
- Prealloc publishes same-block producer records through
  `PreparedEdgePublicationSourceProducerLookups` and
  `PreparedSameBlockScalarProducer` in `select_chain_lookups.hpp` and
  `prepared_lookups.cpp`.
- AArch64 scalar call-argument materialization in `calls.cpp` consumes
  `PreparedCallArgumentSourceProducerMaterialization` through
  `find_prepared_scalar_call_argument_source_producer_materialization` and can
  also read Route 6 BIR producer records before falling back to prepared
  source-producer lookup records.
- The relevant audit/taxonomy rows are
  `TAX-FAM-VALUE-MATERIALIZATION-PLACEHOLDER-001` and idea 415's handoff to
  migrate selected same-block producer chains; no specific 418 recovery row
  names this exact AArch64 scalar producer path, so the local source idea is
  the controlling contract.

Selected family for Step 2:

- AArch64 scalar call-argument same-block binary producer materialization over
  existing prepared source-producer records.

Required typed facts:

- destination/source value identity and type for the produced call argument
- producer block label and producer instruction index
- producer kind `Binary`
- binary opcode and operand values
- materializable-opcode admission from
  `prepared_call_argument_binary_producer_opcode_is_materializable`
- scheduling authority that the producer is in the same block and appears
  before the call instruction

Rejected combinations:

- missing source-producer lookup table or missing producer record
- invalid produced value name or destination identity
- producer instruction at or after the call
- producer instruction index outside the block
- prepared producer record that does not match the BIR instruction at its
  recorded index
- producer kind other than selected `Binary`
- unsupported binary opcode such as unsigned division, shifts, or comparisons
- ambiguous or duplicate Route 6-only producer facts without prepared fallback

Selected consumer migration candidate:

- AArch64 `find_scalar_call_argument_source_producer_materialization` /
  `materialize_scalar_call_argument_value` in
  `src/backend/mir/aarch64/codegen/calls.cpp`, because it already materializes
  scalar binary producers before calls and has focused coverage in
  `backend_aarch64_instruction_dispatch`.

## Suggested Next

Begin Step 2 by adding a typed binary call-argument producer materialization
fact/query around `PreparedCallArgumentSourceProducerMaterialization`, then
cover coherent and rejected payloads with focused tests.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target producer-shape recovery
  intact as incomplete.

## Proof

Lifecycle-only runbook replacement; no build required.

Step 1 inventory-only update; no build required.

Suggested Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier|backend_aarch64_instruction_dispatch)$'`.
