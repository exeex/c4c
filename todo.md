Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within Authorized Terminator Families

# Current Packet

## Just Finished

Step 3 from `plan.md` refreshed the unsupported terminator-fragment residuals
after the Step 2 integer scalar fused-compare branch consumer rule.

The RV64 torture `case.log` files still contain 71 stale
`unsupported_terminator_fragment` rows, including Step 2 positives, so this
packet used direct `build/c4cll --codegen obj --target riscv64-linux-gnu`
probes over those 71 rows for current classification.

Current direct-probe split:
- 20 stale terminator rows now compile to RV64 objects:
  `src/20030403-1.c`, `src/20031201-1.c`, `src/20050104-1.c`,
  `src/20080813-1.c`, `src/20100209-1.c`, `src/20180226-1.c`,
  `src/920501-2.c`, `src/921104-1.c`, `src/950704-1.c`,
  `src/961017-2.c`, `src/961206-1.c`, `src/nestfunc-4.c`,
  `src/pr31169.c`, `src/pr33142.c`, `src/pr38051.c`,
  `src/pr46316.c`, `src/pr48197.c`, `src/pr66940.c`,
  `src/pr80501.c`, and `src/tstdi-1.c`.
- 46 rows still stop at `unsupported_terminator_fragment`.
- 5 stale terminator rows now reroute to downstream non-terminator owners:
  `src/20040703-1.c` to `unsupported_prepared_move_bundle_classification`,
  `src/cmpdi-1.c` to `unsupported_move_bundle_target_shape`,
  `src/950607-2.c` to `unsupported_call_abi`,
  `src/pr20527-1.c` to `unsupported_instruction_fragment`, and
  `src/991221-1.c` to malformed prepared join/consumer evidence.

The 46 current terminator rows split by first prepared fused-compare evidence:
- Authorized floating compare branch candidates: 26 rows, including
  `src/20000731-1.c`, `src/20030910-1.c`, `src/920618-1.c`,
  `src/cmpsf-1.c`, `src/ieee/fp-cmp-1.c`, `src/ieee/fp-cmp-6.c`,
  `src/pr23941.c`, and `src/pr35456.c`.
- Pointer compare branch candidates: 10 rows, including `src/20000801-2.c`,
  `src/20020129-1.c`, `src/20041212-1.c`, `src/20071108-1.c`,
  `src/950621-1.c`, `src/980506-1.c`, `src/loop-15.c`,
  `src/pr28778.c`, `src/pr44555.c`, and `src/pr56962.c`.
- Branch operand authority or mixed integer residuals: 10 rows, including
  `src/20001121-1.c`, `src/20051215-1.c`, `src/980604-1.c`,
  `src/980612-1.c`, `src/ieee/980619-1.c`, `src/ieee/mzero6.c`,
  `src/pr29798.c`, `src/pr58574.c`, `src/pr58831.c`, and
  `src/pr88714.c`. These are not a new integer-predicate consumer family;
  prepared dumps show stack-home/missing-publication or later mixed branch
  evidence that should remain fail-closed until operand authority is explicit.

Adjacent non-terminator first-owner buckets remain separate from Step 3
consumer broadening:
- Branch operand authority: 7 current `unsupported_branch_stack_load_authority`
  rows: `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`, and
  `src/pr39100.c`.
- Branch stack-source freshness: 3 current
  `unsupported_branch_stack_load_source_freshness` rows:
  `src/20060910-1.c`, `src/930930-1.c`, and `src/990127-1.c`.
- Return/call ABI remains outside idea 611 consumer broadening; the stale
  terminator row `src/950607-2.c` now exposes `unsupported_call_abi`.
- Generic instruction-fragment work remains outside idea 611 consumer
  broadening; the stale terminator row `src/pr20527-1.c` now exposes
  `unsupported_instruction_fragment`.

## Suggested Next

Step 3 implementation packet:

Implement the narrow RV64 prepared terminator consumer rule for authorized
scalar floating fused-compare conditional branches. Seed on
`src/20030910-1.c` or `src/920618-1.c`, with nearby breadth on
`src/20000731-1.c`, `src/cmpsf-1.c`, and `src/ieee/fp-cmp-1.c`.

Require explicit prepared floating compare branch evidence plus materializable
FPR/immediate operands, emit only the direct RV64 floating compare plus branch
sequence, and preserve fail-closed behavior for pointer compare, mixed integer
operand-authority, branch authority, freshness, ABI, and instruction-fragment
rows.

Exact proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Watchouts

- Do not infer missing branch operands from final layout or stack homes.
- Keep pointer compare branches out of the next floating compare packet.
- Keep branch operand authority, branch stack-source freshness, ABI, malformed
  join evidence, move-bundle, and generic instruction-fragment rows out of
  Step 3 consumer broadening.
- Keep `src/20000314-3.c`, `src/20001017-1.c`, `src/20050125-1.c`,
  `src/20080519-1.c`, `src/20140828-1.c`, `src/loop-2e.c`,
  `src/pr39100.c`, `src/20060910-1.c`, `src/930930-1.c`, and
  `src/990127-1.c` as negative authority/freshness rows.
- Keep `src/980604-1.c`, `src/pr58574.c`, and `src/pr88714.c` as mixed
  branch/operand-authority watchouts, not as floating-compare proof targets.
- The direct probe path is `build/c4cll`, not `build/src/apps/c4cll`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 346 backend tests, 0 failures. Test subset: `^backend_`.
Proof log: `test_after.log`.

Focused diagnostic probes used direct `build/c4cll --codegen obj --target
riscv64-linux-gnu` commands over the stale 71 terminator rows; no implementation
files, expectations, unsupported markers, allowlists, or lifecycle files other
than `todo.md` were edited.
