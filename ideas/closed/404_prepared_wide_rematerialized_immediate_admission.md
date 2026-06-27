# Prepared Wide Rematerialized Immediate Admission

Status: Closed
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Discovered From: `ideas/closed/401_rv64_object_route_scalar_and_floating_edge_lowering.md`

## Goal

Repair BIR/prepared producer admission for wide rematerialized immediate
values that reach RV64 object emission without a destination home.

## Why This Exists

The 2026-06-26 scalar compare/trunc follow-up for idea 401 moved the in-scope
scalar representatives past `unsupported_scalar_compare_trunc`. The remaining
`tests/c/external/gcc_torture/src/int-compare.c` boundary is not scalar
compare/trunc lowering. Its first reproduced blocker constructs `INT_MIN`
through wide immediate arithmetic such as `bir.sub i32 0, 2147483647` followed
by subtracting `1`.

Those values are prepared as rematerializable immediates with no destination
home, while current prepared pure-instruction admission only skips immediates
that fit the signed 12-bit RV64 immediate form. That is a producer/prepared
admission boundary, not a reason for RV64 object emission to reconstruct BIR
constant semantics.

## In Scope

- Classify wide rematerialized immediate producer forms that are semantically
  complete but lack a prepared destination home.
- Decide whether BIR/prepared lowering should publish a real home, a reusable
  constant-materialization fact, or a stricter admission rejection for these
  producers.
- Repair the producer/prepared handoff so RV64 object emission can consume
  explicit facts instead of rediscovering wide constants from instruction
  shape.
- Prove `src/int-compare.c` and nearby wide-immediate producer cases through
  the backend route.

## Out Of Scope

- Reopening scalar compare/trunc or floating-cast lowering from idea 401.
- Implementing scalar `ashr`, bitfield-global lowering, or other generic RV64
  instruction fragments; those remain with
  `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.
- Teaching RV64 object emission to infer arbitrary BIR constant-expression
  semantics when prepared facts are missing.
- Expectation rewrites, allowlist-only progress, or testcase-specific
  shortcuts.

## Acceptance

- `src/int-compare.c` no longer fails first because a wide rematerialized
  immediate producer lacks an admissible home or prepared fact.
- Nearby cases with the same wide immediate producer family either pass through
  RV64 object emission or are split by a concrete non-duplicate follow-up.
- Backend proof shows the repaired producer route does not regress existing
  immediate, compare, or object-emission coverage.

## Reviewer Reject Signals

- Reject filename-specific handling for `int-compare.c` or special casing
  `INT_MIN`.
- Reject merely increasing an immediate threshold without preserving signedness,
  width, and materialization semantics for the prepared value.
- Reject MIR/RV64 object-emitter code that reconstructs missing BIR constant
  arithmetic instead of consuming producer-published facts.
- Reject claiming progress if the same no-home wide rematerialized immediate
  failure remains behind a renamed diagnostic.
- Reject expectation downgrades, unsupported markers, or allowlist filtering.

## Lifecycle Notes

- 2026-06-27: Closed after implementation commit `29d6c33c` admitted prepared
  rematerialized binary immediates for named I32 non-compare binary producers
  whose result already has explicit `PreparedValueHomeKind::RematerializableImmediate`
  facts with `imm_i32`. The repair kept constant semantics in the
  producer/prepared contract instead of reconstructing arbitrary BIR
  expression semantics in RV64 object emission.
- 2026-06-27: Focused coverage in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` proves a
  traversed wide rematerialized `sub 0, 2147483647` then subtract-one producer
  chain and verifies `INT_MIN` materialization through the existing
  load-immediate path.
- 2026-06-27: Step 3 closure proof commit `296b51b4c` showed
  `tests/c/external/gcc_torture/src/int-compare.c` now reports
  `dump_bir_status=0`, `prepared_status=0`, `codegen_obj_status=0`, and
  `runner_status=0`; the representative runner reported `total=1 passed=1
  failed=0`.
- 2026-06-27: The old `RV64_C4C_OBJ_COMPILE_FAIL` /
  `unsupported_instruction_fragment` for `%t6/%t7`-style rematerialized I32
  wide-immediate producer chains is absent, and no fresh residual appeared in
  the representative proof.
- 2026-06-27: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
