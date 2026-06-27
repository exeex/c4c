# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Selected Consumers

## Just Finished

Completed Step 4 selected consumer migration for rematerializable integer
immediates.

Current value-home payloads live in `PreparedValueHome`:

- `RematerializableImmediate` currently carries `immediate_i32` and optional
  `immediate_f128`, but no dedicated typed materialization contract that names
  width, signedness, value identity, or target range.
- `PointerBasePlusOffset` currently reuses value-home fields for register/base
  identity, pointer base value/symbol, and byte delta.
- Same-block producer-chain materialization is spread across prepared producer
  lookup records and target helpers rather than a single materialization fact.

Primary target-side recovery sites:

- RV64 `prepared_scalar_emit.cpp` uses
  `prepared_immediate_i32_for_value` and
  `simple_or_prepared_integer_immediate` for rematerialized integer operands.
- RV64 `prepared_scalar_emit.cpp` also searches same-block select/compare
  producers and pointer arithmetic producers near the current BIR instruction.
- AArch64 `calls.cpp` materializes scalar call-argument producer chains through
  Route 6/source-producer records with fallback source-producer lookup
  generation.
- AArch64 `alu.cpp` asks same-block producer helpers whether publication may
  need scratch registers, including binary/select/load producers.

Selected first family for Step 2: `RematerializableImmediate` integer facts.
This is narrower than pointer base-plus-offset or producer-chain scheduling and
has clear existing consumers in RV64 scalar emission.

Required typed facts for the first family:

- source value id/name and function identity
- integer immediate payload
- scalar width and signed interpretation used by the target
- target range/admission status for consumers that need signed 12-bit or wider
  materialization

Rejected combinations:

- rematerializable immediate home without an integer payload
- immediate payload without value identity
- target consumer using a rematerialized immediate with no width/signedness
  fact
- fallback from absent prepared immediate facts to nearby producer-shape
  recovery

Step 2 implementation:

- Added `PreparedRematerializableIntegerImmediateFact` as a typed view over
  coherent `PreparedValueHomeKind::RematerializableImmediate` integer homes.
- The query requires source value id, function name, value name, signed i32
  payload, and no cross-family f128 payload.
- The query exposes 32-bit signed interpretation and signed-12 target range
  admission for consumers that need immediate-field legality.
- Added focused tests for coherent facts, large-but-coherent facts, missing
  payload, missing identity, cross-family payload, and wrong home kind.
- Created `docs/prepared_fact_contracts/value_materialization_contract_plan.md`
  to track this materialization family.

Step 3 implementation:

- Added `PreparedRematerializableIntegerImmediateContractStatus` for coherent,
  missing, and incoherent selected-family records.
- Added `verify_prepared_rematerializable_integer_immediate_contract` with the
  `value_materialization_fact` fact family.
- Missing value home, value id, function name, value name, or i32 payload map
  to `producer_missing`.
- Wrong home kind and cross-family f128 payload map to `producer_incoherent`.
- Focused verifier tests cover coherent reports, missing payload, missing
  identity, wrong kind, cross-family payload, absent home, status spelling, and
  fact-family spelling.

Step 4 implementation:

- Migrated RV64 prepared scalar immediate lookup to the typed verifier/query.
- Migrated RV64 object integer immediate lookup to the typed verifier/query.
- Migrated RV64 before-return rematerialized immediate moves to the typed
  verifier/query and its signed-12 target-admission fact.
- Migrated RV64 rematerializable binary-result detection to the typed
  verifier/query.
- Added an RV64 object rejection test proving an invalid prepared immediate
  identity fails closed instead of being consumed from raw `immediate_i32`.

## Suggested Next

Begin Step 5 with broader validation for the rematerializable integer immediate
family and decide whether the next materialization family needs a follow-up
runbook.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target recovery intact as
  incomplete.

## Proof

Step 2 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prealloc_decoded_home_storage$'`.

Result: passed, 1/1 tests.

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS; before 1/1, after 1/1, no new failing tests.

Step 3 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_decoded_home_storage|backend_prealloc_prepared_contract_verifier)$'`.

Result: passed, 2/2 tests.

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS; before 2/2, after 2/2, no new failing tests.

Step 4 focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_decoded_home_storage|backend_prealloc_prepared_contract_verifier|backend_riscv_object_emission|backend_riscv_prepared_edge_publication)$'`.

Result: passed, 4/4 tests.

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS; before 4/4, after 4/4, no new failing tests.
