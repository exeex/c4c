# Prepared Local Address Base-Plus-Offset Boundary Evidence

Status: Closed
Type: Focused producer/consumer evidence gap
Parent: `ideas/closed/547_bir_local_memory_call_metadata_boundary_review.md`
Owning Layer: Prepared contract and RV64 prepared local-memory consumer

## Goal

Prove whether retained `prepared_local_address_base_plus_offset_missing`
`unsupported_local_memory_access` rows are prepared-owned or RV64-owned before
any local-memory lowering repair proceeds.

## Why This Exists

The boundary review retained 41 local-memory rows from per-case RV64 torture
logs. Representative cases such as `src/20000519-1.c`, `src/20070212-3.c`,
`src/pr35800.c`, and `src/pr65369.c` reached the RV64 object route and failed
because RV64 required a prepared frame-slot or pointer-value
base-plus-offset local-memory address.

Step 3 could not honestly assign first ownership because the retained logs do
not include focused prepared dumps that prove whether the prepared
base-plus-offset fact is absent/incoherent or present and rejected by RV64.

## In Scope

- Rerun one retained representative, preferably `src/20000519-1.c`, through
  semantic BIR, prepared BIR, and RV64 object routes in the same workspace
  snapshot.
- Inspect BIR `load_local`/`store_local` address records and prepared
  `memory_access` / `address_materialization` rows.
- Prove `PreparedAddressBaseKind`, `can_use_base_plus_offset`, size,
  alignment, default-space/non-volatile status, pointer-value register homes,
  and immediate encodability for the failing access.
- Classify the first owner:
  - prepared-owned if the required prepared base-plus-offset fact is missing
    or incoherent
  - RV64-owned if the required prepared fact is present and
    `fragment_for_prepared_store_local` /
    `fragment_for_prepared_load_local` still rejects it
- Create or execute a repair only after the first bad fact is named.

## Out Of Scope

- RV64 target-shaped recovery from raw LIR/BIR address expressions.
- Combining this route with direct-call metadata repair.
- Reclassifying latest summary-file rows without a fresh matching scan.
- Weakening unsupported accounting, tests, or prepared admission contracts.

## Acceptance Criteria

- A focused proof log names the representative case, exact failing access, and
  first bad prepared/RV64 fact.
- The owner classification is tied to visible semantic BIR, prepared BIR, and
  RV64 evidence from the same run.
- If a repair is made, it adds focused BIR/prepared coverage first and then
  proves representative RV64 behavior without expectation downgrades.
- The old `unsupported_local_memory_access` failure mode is either removed by
  the named capability repair or explicitly remains blocked with the missing
  evidence recorded.

## Completion Notes

Closed on 2026-07-03 after Step 5 completed the active runbook.

- The representative `tests/c/external/gcc_torture/src/20000519-1.c` now emits
  an RV64 object with:
  `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c -o build/agent_state/561_step5_20000519_1/rv64.o`.
- The Step 1 prepared pointer-value stack-home local-memory first bad fact no
  longer blocks object emission, and Step 5 did not expose a new first bad
  prepared/RV64 fact for the representative.
- The focused repair path was proven by the executor-updated Step 5 packet and
  the supervisor-selected backend subset.
- Close-gate regression guard passed for the backend subset:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
  The guard reported `before: passed=345 failed=0 total=345` and
  `after : passed=345 failed=0 total=345`.

No separate leftover lifecycle blocker remains for this idea.

## Reviewer Reject Signals

- Reject any RV64 lowering that reconstructs frame-slot, pointer-value, or
  offset facts from target-specific instruction shapes instead of consuming
  prepared facts.
- Reject claiming prepared ownership without a focused prepared dump showing a
  missing or incoherent `PreparedMemoryAccess` / address-materialization fact.
- Reject claiming RV64 ownership without proof that the required prepared
  base-plus-offset fact is present and still rejected by the RV64 consumer.
- Reject testcase-shaped shortcuts for `src/20000519-1.c`,
  `src/20070212-3.c`, `src/pr35800.c`, `src/pr65369.c`, or any other named
  torture case.
- Reject expectation rewrites, unsupported downgrades, diagnostic renames, or
  helper-only refactors as local-memory capability progress.
- Reject broad local-memory rewrites that do not name the first bad fact at
  the prepared/RV64 boundary.
