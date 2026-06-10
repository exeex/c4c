Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Prepared Publication Surface And Evidence Set

# Current Packet

## Just Finished

Completed Step 1 evidence inventory for `plan.md`: mapped the prepared
current-block publication identity surface, direct consumers, rejected
target/prealloc-policy fields, candidate equivalence cases, and the first
narrow code-changing proof route.

Inspected files/functions:
- `src/backend/prealloc/select_chain_lookups.hpp`: owns
  `PreparedCurrentBlockPublicationConsumption` and declares
  `find_prepared_current_block_publication_consumption`.
- `src/backend/prealloc/prepared_lookups.cpp`: defines
  `find_prepared_current_block_publication_consumption` and both
  `find_prepared_current_block_entry_publication` overloads.
- `src/backend/prealloc/value_locations.hpp`: owns
  `PreparedBlockEntryPublication`,
  `PreparedCurrentBlockEntryPublicationQueryInputs`,
  `PreparedCurrentBlockEntryPublication`,
  `prepared_block_entry_publication_available`, and
  `collect_prepared_block_entry_publications`.
- `src/backend/prealloc/publication_plans.hpp/.cpp`: scalar publication plans
  carry `current_block_entry_publication` and
  `current_block_entry_publication_available` beside hook/home/storage fields.
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`: consumes current
  block entry publications through `collect_current_block_entry_publications`,
  `value_has_current_block_entry_publication`,
  `current_block_entry_publication_register`, and
  `record_current_block_entry_publication_registers`.
- `src/backend/mir/aarch64/codegen/calls.cpp`: direct caller
  `prepared_call_boundary_source_value` uses
  `find_prepared_current_block_publication_consumption`; fallback
  `call_boundary_source_value_by_name` then checks same-block named producers.
- Existing evidence tests inspected:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`, and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

Direct caller map:
- `find_prepared_current_block_publication_consumption` is called by
  `prepared_call_boundary_source_value` in AArch64 calls and by existing
  prepared helper/contract tests.
- `find_prepared_current_block_entry_publication` is called by
  `current_block_entry_publication_register` in AArch64 dispatch publication
  and by the block-entry publication test.
- `collect_prepared_block_entry_publications` is called by
  `collect_current_block_entry_publications`,
  `value_has_current_block_entry_publication`, prepared printer coverage, x86
  prepared query reuse, and block-entry publication tests.

Semantic fields to move toward BIR identity:
- Query key: `block_label`, `block`, `value_name`, and
  `before_instruction_index`; for entry publication, `successor_label` plus
  destination BIR/prepared value identity.
- Result identity: `available`, `source_producer`, `instruction`,
  `produced_value`, `instruction_index`, `value_name`, and
  `source_producer_kind`.
- Entry semantic identity can reuse destination value id/name availability as
  lookup evidence, but BIR should model value availability/source identity
  rather than the prepared move payload.

Rejected fields for BIR publication identity:
- Hook/policy: `PreparedScalarPublicationHookKind`, scalar publication status
  policy, immediate publication payloads, and scalar emission decisions.
- Home/storage: `PreparedValueHome`, `destination_home`,
  `destination_home_kind`, `PreparedStorageEncodingKind`, slot/offset/size/align
  fields, and storage kind checks.
- Register/emission: `destination_register_name`, AArch64 register parsing,
  `RegisterOperand`, register view conversion, emitted scalar-register state,
  and `emit_*publication*` helpers.
- Move/planning payload: `PreparedMoveBundle`, `PreparedMoveResolution`,
  move phase/authority, destination kind/storage, occupied registers, and
  publication ordering.

Candidate equivalence cases:
- Entry available: block-entry publication for a matching successor/value is
  available; existing coverage in `backend_prealloc_block_entry_publications`
  and AArch64 dispatch indexed-value test.
- Same-block available: current-block consumption finds `%sum` before the end
  of the block and returns source producer kind `Binary`, produced value,
  instruction index, and value name; covered in prepared lookup helper and call
  contract tests.
- Unavailable/missing: null lookup inputs, invalid block/value names, missing
  destination home, missing publication, unsupported storage, and missing
  register name all fail closed without fabricating identity.
- Wrong value: named BIR value lookup must resolve through prepared names and
  fail closed for unknown/missing value names.
- Wrong block: `producer->block_label != block_label` in
  `find_prepared_current_block_publication_consumption` must fail closed; add a
  BIR/prepared equivalence assertion for a copied producer fact under another
  block label.
- Before-index/future producer: producer instruction index greater than or
  equal to `before_instruction_index` fails closed; existing coverage uses
  `%product` before index `7` and `%sum` before index `1`.
- Wrong producer fact: mismatched producer instruction index or producer/result
  mismatch fails closed; existing prepared lookup helper and call contract tests
  mutate source producer lookups to prove this.

## Suggested Next

First code-changing packet: add BIR-side block-entry/current-block publication
identity request/result records and a test-only comparison helper that mirrors
`PreparedCurrentBlockPublicationConsumption` for semantic identity fields only.
Place the helper near existing BIR/prepared same-block query comparison
coverage, then assert same-block available, unavailable, wrong-value,
wrong-block, before-index, and mismatched-producer cases. Do not switch
consumers and do not read hook/home/storage/register/emission fields from BIR.

## Watchouts

- Keep prepared current-block publication queries available as the oracle until
  BIR equivalence is proven.
- Model only source/value/producer/instruction identity in BIR; do not carry
  `PreparedBlockEntryPublication` wholesale.
- Do not import hook kind, destination home, storage encoding, stack-source
  extension policy, register-view conversion, immediate publication payloads,
  emitted storage availability, or scalar publication emission policy into BIR.
- Entry-publication lookup currently includes prepared destination homes and
  move/register payload because the AArch64 consumer needs registers. The BIR
  packet should compare only semantic availability/destination identity and
  leave register parsing in `dispatch_publication.cpp`.
- Escalate validation if dispatch publication, calls, scalar publication
  planning, value homes, or emission behavior changes.

## Proof

Evidence-only packet; no build or tests required and no `test_after.log`
written.

Proposed first code-changing proof command:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test backend_prealloc_block_entry_publications_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_block_entry_publications)'
```

If the first packet touches AArch64 dispatch-publication consumer behavior,
extend the same proof with:

```sh
cmake --build build --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R 'backend_aarch64_instruction_dispatch|backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_block_entry_publications)'
```
