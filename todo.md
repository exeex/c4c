# Current Packet

Status: Active
Source Idea Path: ideas/open/415_prepared_value_materialization_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the First Typed Materialization Fact

## Just Finished

Completed Step 2 typed materialization fact for rematerializable integer
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

## Suggested Next

Begin Step 3 by adding producer-side verifier statuses and reports for
`PreparedRematerializableIntegerImmediateFact`, including missing payload,
missing identity, and cross-family payload cases.

## Watchouts

- Do not add target-local evaluators for arbitrary BIR expressions.
- Do not weaken runtime comparison, gcc_torture, or supported-path tests.
- Treat diagnostics-only changes that leave target recovery intact as
  incomplete.

## Proof

Focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prealloc_decoded_home_storage$'`.

Result: passed, 1/1 tests.

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS; before 1/1, after 1/1, no new failing tests.
