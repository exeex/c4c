Status: Active
Source Idea Path: ideas/open/159_bir_producer_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The BIR Producer Vocabulary And Records

# Current Packet

## Just Finished

Step 2 - Define The BIR Producer Vocabulary And Records completed as the first
code-changing packet.

Added BIR-owned Route 1 schema surface in `src/backend/bir/bir.hpp` /
`src/backend/bir/bir.cpp`:

- `Route1ProducerKind` with `Unknown`, `Immediate`, `LoadLocal`, `LoadGlobal`,
  `Cast`, `Binary`, and `SelectMaterialization`.
- `Route1SourceValueIdentity` for BIR value pointer, kind, name, name id, type,
  immediate integer constant, and pointer-symbol identity.
- `Route1ProducerInstructionIdentity` for producer instruction pointer,
  instruction index, producer kind, block label, and block label id.
- `Route1ImmediateIntegerConstant` and `Route1MaterializationAvailability` as
  typed annotation payloads.
- `Route1ProducerRecord` plus BIR-local helpers to derive records from a block
  instruction without calling prepared producer authority.

Extended `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` to cover
the new records in the existing same-block scalar producer fixture: immediate
constant identity, producer instruction/index, source name/type identity,
materialization availability, and fail-closed behavior for missing/non-producer
instructions. No production consumers were switched.

## Suggested Next

Execute Step 3 as the next code-changing packet:

- Add BIR same-block producer query APIs and cheap lookup/index surfaces that
  point at the new `Route1ProducerRecord` / typed annotation records.
- Keep prepared helpers as oracle checks only; do not switch AArch64 or broad
  production consumers.
- Compare BIR-backed query answers with prepared same-block producer,
  integer-constant, and materialization-oracle answers in
  `backend_prepared_lookup_helper`.

## Watchouts

- Step 2 intentionally did not move existing MIR same-block queries onto the
  new BIR records; Step 3 owns query/index integration.
- `Route1ProducerRecord` currently derives from individual BIR instructions and
  does not yet represent immediate values as standalone producer records.
  Immediate constants are represented by `Route1SourceValueIdentity` and
  `Route1ImmediateIntegerConstant`.
- Keep materialization payload semantic only. Do not add prepared homes,
  registers, storage, frame slots, emitted availability, spill/reload behavior,
  operand views, or final instruction records.
- `clang-format` is not installed in this environment; manual style hygiene was
  used for this packet.

## Proof

Ran exact delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Full proof output saved in `test_after.log`.
