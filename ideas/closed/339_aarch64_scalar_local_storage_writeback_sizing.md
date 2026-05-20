# AArch64 Scalar Local Storage Writeback Sizing

Status: Closed
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 scalar local storage and writeback for non-address-exposed
scalar locals whose prepared stack access size or machine lowering currently
causes frame-size-zero slots, uninitialized loads, or missing writeback.

## Why This Exists

The post-338 backend-regex inventory rejected a broad scalar integer/FP
runtime-value owner as too large. The first bad facts do not collapse across
integer locals, FP comparisons, pointer/null conditionals, FP expression/call
materialization, and return-spill materialization.

The focused shared owner is the local scalar storage/writeback bucket for
`00086` and `00111`:

- `00086` prepares a short local update through `store_local`, `load_local`,
  `add`, `trunc`, `store_local`, and compare, but generated AArch64 assembly
  allocates frame size 0, reads `[sp]`, computes the increment, never stores
  the updated short, and reloads `[sp]` for the compare.
- `00111` stores only the `long l = 1`, reads short `s` before initializing
  it, computes `s - l`, never writes the result back to `s`, and returns a
  reload of the original short slot.

This points at prepared stack-object/access sizing and local scalar
load/store writeback lowering, not scalar arithmetic, FP comparison, pointer
conditional, or return ABI materialization as a whole.

## In Scope

- Localize why non-address-exposed scalar locals can receive zero-size or
  inconsistent prepared stack-object/access sizing.
- Repair AArch64 lowering for scalar `store_local` and `load_local` paths so
  short-sized locals are initialized, updated, and reloaded consistently.
- Cover local scalar initialization, promotion/truncation, compound
  assignment writeback, and compare/return use of the updated local value.
- Add or update focused backend coverage for scalar local slot sizing and
  writeback behavior without relying only on c-testsuite filenames.
- Prove the focused external cases `00086` and `00111` advance past the
  current runtime residual or pass.

## Out Of Scope

- FP comparison result materialization or FP expression/call argument lowering
  for `00119`, `00123`, or `00174`.
- Pointer-null conditional conversion and pointer/local conditional buckets
  such as `00112` and `00144`.
- Broad scalar return-spill or ABI return materialization such as `00200`.
- Aggregate, address-exposed local, pointer-address, variadic, timeout,
  runner, expectation, unsupported-classification, or CTest-registration work.
- Fixing only one c-testsuite filename, source line, register number, stack
  offset, emitted instruction spelling, or diagnostic string.

## Acceptance Criteria

- The zero-size or inconsistent prepared stack-object/access fact for
  representative `i16` local accesses is localized to a concrete producer,
  normalizer, handoff, or AArch64 lowering boundary.
- Focused backend coverage proves non-address-exposed scalar locals carry
  usable size/alignment and produce stable local load/store writeback.
- `00086` generated assembly shows either an initialized short slot with the
  incremented value written back and used by the compare, or an equivalent
  register-resident lowering that preserves the same semantics.
- `00111` generated assembly shows initialized `s`, `s -= l` writeback, and
  return of the updated short value, or an equivalent register-resident
  lowering.
- The focused c-testsuite subset for `00086` and `00111` advances without
  expectation, runner, timeout, unsupported, or CTest-registration changes.
- Any remaining failures in the focused cases are reclassified by their new
  first bad fact before this idea is closed.

## Closure Evidence

Closed: 2026-05-20

The active runbook completed localization, repair, and focused runtime proof
for the scoped AArch64 scalar local storage/writeback owner. Step 3 recorded
that both focused runtime representatives,
`c_testsuite_aarch64_backend_src_00086_c` and
`c_testsuite_aarch64_backend_src_00111_c`, passed after the scalar local
sizing/writeback repair, with no residual first bad fact remaining inside this
owner.

Close-time regression guard used matching canonical logs generated for the
same scope:

`ctest --test-dir build -j --output-on-failure -R 'backend_prepare_stack_layout|backend_aarch64_(machine_printer|instruction_dispatch)|c_testsuite_aarch64_backend_src_(00086|00111)_c'`

Guard command:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: PASS. Before was 3 passed / 2 failed / 5 total; after was 5 passed /
0 failed / 5 total. The guard resolved
`c_testsuite_aarch64_backend_src_00086_c` and
`c_testsuite_aarch64_backend_src_00111_c` with no new failing tests.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00086`, `00111`, one local variable name, one source line,
  one stack offset, one register, one emitted instruction sequence, or one
  compare/return shape instead of repairing scalar local sizing/writeback
  generally;
- weakens expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration to reduce the
  failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- broadens into FP compare/expression lowering, pointer/null conditionals,
  broad return-spill ABI materialization, aggregates, variadics, or
  address-exposed local memory without fresh evidence that the local
  sizing/writeback first bad fact moved there;
- leaves non-address-exposed scalar local accesses carrying zero or
  inconsistent size facts behind a renamed abstraction;
- proves only one representative while nearby same-owner scalar local
  initialization/writeback behavior remains unexamined.
