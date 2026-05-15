Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Select I128 Arithmetic And Bitwise Nodes

# Current Packet

## Just Finished

Step 4 selected supported i128 add/sub/bitwise pair operation records from
complete `PreparedI128Carrier` register-pair facts, without shifts,
comparisons, helper-call lowering, or printer scope.

Completed work:

- Added `I128PairOperationRecord` selected machine-node records carrying
  operation kind, lane semantics, source BIR opcode, result/source carrier
  identities, low/high lane order, lane width, total size/alignment, and
  result/source register-pair operands.
- Added prepared conversion for supported i128 `Add`, `Sub`, `And`, `Or`, and
  `Xor` binaries, with explicit carry-propagating, borrow-propagating, or
  independent-bitwise lane semantics.
- Added fail-closed diagnostics for unsupported opcodes, non-i128 types,
  unnamed values, missing carriers, incomplete carriers, memory-backed carriers
  in arithmetic selection, and register conversion failures.
- Wired AArch64 dispatch to select i128 pair operations before scalar lowering,
  so i128 binaries are consumed from structured carrier facts rather than
  scalar register inference.
- Added focused record and dispatch tests covering i128 add, sub, bitwise
  selection, source/result lane preservation, and missing-carrier diagnostics.

## Suggested Next

Execute Step 5 from `plan.md`: select supported i128 shift and comparison
nodes from complete prepared carrier facts. Keep helper-call lowering and
terminal printer output deferred to later planned packets.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names, numeric
  adjacency, or fixed `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- Current pair-operation selection intentionally requires register-pair
  carriers. Memory-backed i128 arithmetic remains fail-closed until an
  explicit transport or memory arithmetic policy is specified.
- Shifts and comparisons need their own structured semantics for shift counts,
  scalar boolean results, and signed/unsigned high-lane behavior; do not reuse
  the add/sub/bitwise record shape without those fields.
- ABI/helper-call pair facts are still separate. If shift/compare or helper
  selection needs call-boundary pair policy, stop and report the missing
  prepared/shared policy instead of inventing it in target codegen.
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
