# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Pointer Materialization Boundaries

## Just Finished

Mapped current `PointerBasePlusOffset` producer and consumer boundaries.

Producer boundary:

- Regalloc value-home construction in
  `src/backend/prealloc/regalloc/value_homes.cpp` creates
  `PreparedValueHomeKind::PointerBasePlusOffset` for pointer values with
  semantic pointer-carrier authority and a non-identity base/delta pair.
- Pointer carrier state in
  `src/backend/prealloc/regalloc/pointer_carriers.cpp` supplies
  `base_value_name`, optional `base_symbol_name`, `byte_delta`, `step_bytes`,
  and carrier authority from prepared pointer access, frame-address
  materialization, BIR pointer symbols, or immediate pointer offsets.
- Publication/storage planning copies raw pointer payloads into scalar
  publication and store-source plans; decoded home storage currently reports
  pointer homes as unsupported typed operand storage rather than a typed
  materialization fact.

Required typed pointer facts:

- destination prepared value id, function name, and destination value name
- base value name
- optional base symbol name
- signed pointer byte delta, including zero
- target admission facts for direct base-register materialization and
  signed-12 add-immediate materialization

Rejected combinations:

- missing destination function name or value name
- wrong value-home kind
- missing base value name
- missing byte delta
- cross-family rematerialized immediate payload on a pointer home
- pointer consumer fallback that materializes from raw BIR pointer arithmetic
  when the prepared pointer fact is absent or incoherent

Selected consumer migration candidate:

- RV64 edge publication in
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`, because
  it directly accepts `PointerBasePlusOffset` source homes by reading
  `pointer_base_value_name` and `pointer_byte_delta`, resolves the base
  register, and emits `mv`/`addi`/wide-delta sequences from those raw fields.

## Suggested Next

Begin Step 2 by adding `PreparedPointerBasePlusOffsetFact` and a compatibility
query in `src/backend/prealloc/value_locations.hpp`, then cover coherent and
rejected payloads in `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp`.

## Watchouts

- Do not add target-local evaluators for arbitrary pointer expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target pointer recovery intact as
  incomplete.

## Proof

Step 1 inventory-only update; no build required.

Suggested Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prealloc_decoded_home_storage$'`.
