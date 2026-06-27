# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Selected Pointer Consumer

## Just Finished

Implemented Step 4 selected RV64 pointer consumer migration.

Step 4 implementation:

- Migrated RV64 edge-publication pointer source materialization in
  `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp` from raw
  `pointer_base_value_name`/`pointer_byte_delta` reads to
  `verify_prepared_pointer_base_plus_offset_contract` plus
  `as_pointer_base_plus_offset_fact`.
- Preserved existing target checks for resolving the prepared base value to a
  register home, destination-register support, zero-delta moves, signed-12
  add-immediate materialization, and large-delta materialization.
- Added RV64 fail-closed coverage for missing destination identity and
  cross-family immediate payloads, in addition to existing missing base,
  missing delta, unresolved base, aliasing, and destination-form coverage.
- Updated
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md` with
  the migrated pointer consumer.

Step 3 implementation:

Step 3 implementation:

- Added `PreparedPointerBasePlusOffsetContractStatus` for coherent, missing,
  and incoherent pointer materialization records.
- Added `classify_prepared_pointer_base_plus_offset_contract` and
  `verify_prepared_pointer_base_plus_offset_contract` using the
  `value_materialization_fact` fact family.
- Missing value home, function name, destination value name, base value name,
  or pointer byte delta map to `producer_missing`.
- Wrong home kind and cross-family immediate payloads map to
  `producer_incoherent`.
- Focused verifier tests cover coherent reports, missing/invalid base, missing
  delta, missing identity, wrong kind, cross-family i32/f128 payloads, absent
  home, and status spelling.
- Updated
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md` with
  the pointer producer-verification contract.

Step 2 implementation:

Step 2 implementation:

- Added `PreparedPointerBasePlusOffsetFact` as a typed view over coherent
  `PreparedValueHomeKind::PointerBasePlusOffset` homes.
- The query preserves destination value id, function name, destination value
  name, base value name, optional base symbol name, and signed byte delta.
- The query exposes target-admission facts for zero-delta direct base-register
  copy and signed-12 pointer byte deltas.
- The query rejects missing destination identity, missing or invalid base value
  name, missing byte delta, wrong home kind, and cross-family immediate
  payloads.
- Updated
  `docs/prepared_fact_contracts/value_materialization_contract_plan.md` with
  the pointer materialization fact contract.
- Added focused fact-query tests for coherent pointer facts, zero and large
  deltas, missing base/delta/identity, cross-family payloads, and wrong kind.

Step 1 boundary summary:

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

Run the Step 4 focused proof command, then broaden validation in Step 5.

## Watchouts

- Do not add target-local evaluators for arbitrary pointer expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target pointer recovery intact as
  incomplete.

## Proof

Step 1 inventory-only update; no build required.

Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prealloc_decoded_home_storage$'`.

Result: passed, 1/1 tests.

Step 3 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_decoded_home_storage|backend_prealloc_prepared_contract_verifier)$'`.

Result: passed, 2/2 tests.

Step 4 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_decoded_home_storage|backend_prealloc_prepared_contract_verifier|backend_riscv_prepared_edge_publication)$'`.

Result: passed, 3/3 tests.
