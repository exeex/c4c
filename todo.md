Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Select I128 Shift And Comparison Nodes

# Current Packet

## Just Finished

Step 5 selected supported AArch64 i128 shift and comparison records from
complete prepared carrier and scalar value facts, without helper-call lowering
or printer scope.

Completed work:

- Added selected `I128ShiftRecord` nodes carrying shift kind, structured
  shift-count operand, count kind, cross-lane shift behavior, source/result
  carrier identities, low/high lane order, lane width, and total size/alignment.
- Added selected `I128CompareRecord` nodes carrying predicate, signedness,
  high-word comparison semantics, scalar boolean result register facts,
  source carrier identities, and low/high lane order.
- Added prepared conversion for `Shl`, `LShr`, and `AShr` i128 shifts with
  immediate or prepared scalar register counts, plus representative signed and
  unsigned i128 comparisons with prepared scalar result authority.
- Kept unsupported shift/count/comparison states, missing pair carriers,
  memory-backed carriers, missing scalar comparison result facts, and register
  conversion failures fail-closed through explicit diagnostics.
- Added focused record and dispatch tests for shift lane semantics, signed and
  unsigned comparison high-word semantics, scalar comparison result authority,
  and missing-result diagnostics.

## Suggested Next

Execute Step 6 from `plan.md`: add runtime helper boundary records for i128
families that require helper calls, or record the exact missing prepared
argument/result/clobber authority if helper facts are not complete.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names, numeric
  adjacency, or fixed `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- Shift and comparison selection intentionally requires register-pair i128
  carriers. Memory-backed i128 shift/compare remains fail-closed until an
  explicit transport or memory operation policy is specified.
- Shift records carry count operands, but terminal expansion is still deferred;
  do not add printer templates or recover operands from rendered names.
- Comparison records require prepared scalar result value-home/storage facts;
  missing boolean result authority must stay fail-closed.
- Runtime helper boundary work needs structured argument, result, and clobber
  facts. If those facts are missing, report the prepared/shared blocker instead
  of inventing fixed helper registers in AArch64 codegen.
- Keep binary128/F128 and float/i128 conversions separate until helper-call
  facts carry explicit register-bank transition authority.

## Proof

Proof command run:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

Result: passed, 5/5 selected tests green. Proof log: `test_after.log`.

Supervisor full-suite acceptance also passed: `(cmake --build build -j2 &&
ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`, with
regression guard accepted against full-suite `test_before.log` copied from
accepted `test_baseline.log`; 3167/3167 before and 3167/3167 after.

Additional hygiene: `git diff --check` passed.
