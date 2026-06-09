Status: Active
Source Idea Path: ideas/open/146_call_argument_materialization_call_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Call-Argument Materialization Ownership

# Current Packet

## Just Finished

Completed Step 1 inspection for call-argument materialization ownership.

Current declaration sites:
- `PreparedCallArgumentSourceProducerMaterialization` is still declared in
  `src/backend/prealloc/prepared_lookups.hpp:28`.
- `prepared_call_argument_binary_producer_opcode_is_materializable` is still
  declared in `src/backend/prealloc/prepared_lookups.hpp:113`.
- `find_prepared_call_argument_source_producer_materialization` is still
  declared in `src/backend/prealloc/prepared_lookups.hpp:117`.

Current definition sites:
- `prepared_call_argument_binary_producer_opcode_is_materializable` is defined
  in `src/backend/prealloc/prepared_lookups.cpp:2315`.
- `find_prepared_call_argument_source_producer_materialization` is defined in
  `src/backend/prealloc/prepared_lookups.cpp:2348`.

Direct consumers that name the API today:
- `src/backend/mir/aarch64/codegen/calls.cpp:6287` names
  `PreparedCallArgumentSourceProducerMaterialization` in the return type of
  `find_prepared_scalar_call_argument_source_producer_materialization`.
- `src/backend/mir/aarch64/codegen/calls.cpp:6300` calls
  `find_prepared_call_argument_source_producer_materialization`; AST confirms
  that wrapper is called by `materialize_scalar_call_argument_value` at
  `src/backend/mir/aarch64/codegen/calls.cpp:6309`.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:3855`,
  `:3873`, `:3911`, and `:3922` call
  `find_prepared_call_argument_source_producer_materialization` directly for
  load-local, materializable binary, non-covered binary, and future-producer
  fail-closed coverage.

Same-block/source-producer dependency owner from prior work:
- Public owner is `src/backend/prealloc/select_chain_lookups.hpp`, which owns
  `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`,
  `find_prepared_same_block_scalar_producer`, and
  `find_prepared_current_block_publication_consumption`.
- Current same-block definitions still live in
  `src/backend/prealloc/prepared_lookups.cpp`; AST confirms the call-argument
  finder directly calls `find_prepared_same_block_scalar_producer` and the
  binary-opcode predicate.
- Step 2 should make `calls.hpp` depend on the select-chain owner for
  `PreparedSameBlockScalarProducer` rather than copying same-block scans into
  call ownership.

Destination assessment:
- `src/backend/prealloc/calls.hpp` is already the public call-facing header and
  `src/backend/prealloc/call_plans.cpp` is the existing implementation owner
  for non-inline declarations in `calls.hpp`.
- A new companion implementation file is not required for this move. If later
  review wants a smaller file, the backend CMake prealloc glob would allow one,
  but the current repo-local pattern is sufficient: move public declarations to
  `calls.hpp` and move definitions into `call_plans.cpp`.

## Suggested Next

Step 2 packet: move `PreparedCallArgumentSourceProducerMaterialization`,
`prepared_call_argument_binary_producer_opcode_is_materializable`, and
`find_prepared_call_argument_source_producer_materialization` declarations from
`prepared_lookups.hpp` to `calls.hpp`; include `select_chain_lookups.hpp` from
`calls.hpp` for `PreparedSameBlockScalarProducer`, and leave implementation
files untouched until Step 3.

## Watchouts

- Keep this route limited to call-argument materialization ownership.
- Do not duplicate same-block materialization logic in call ownership.
- Do not turn the lookup into an AArch64-only shortcut.
- Do not mix ABI move-bundle work into this packet.
- `prepared_lookups.hpp` already includes `calls.hpp`, so moving declarations
  only to `calls.hpp` should avoid leaving a reverse declaration dependency in
  prepared lookup ownership.
- `src/backend/mir/aarch64/codegen/calls.cpp` and the stack-call contract test
  currently include prepared lookup headers for other APIs too; do not remove
  those includes in Step 2 unless the file no longer needs unrelated prepared
  lookup declarations.

## Proof

Inspection-only packet. No build or tests required by supervisor proof. Used
`c4c-clang-tool`/`c4c-clang-tool-ccdb` for declaration, definition, signature,
callee, and caller checks; no `test_after.log` was produced because no proof
command was delegated for this non-code slice.
