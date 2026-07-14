# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.2
Current Step Title: Publish representative scalar compare result/use authority

## Just Finished

- Completed Plan Step 7.1 for the representative explicit scalar integer
  `LirCastOp` result/use chain.
- Added an operand-returning explicit-cast seam: a width-changing scalar integer
  cast allocates its result with `fresh_value`, preserves its source
  `LirOperand`, and returns the exact result operand to a later same-type use.
- Preserved native SExt and exact i32/i64 endpoint refs; verifier coherence now
  requires authoritative integer casts to use Trunc for narrowing or ZExt/SExt
  for widening.
- Added focused exact-ID and misleading-display positives plus invalid/
  duplicate result, unknown/cross-function use, invalid kind, and missing or
  conflicting from/to type rejections. Other cast producers remain
  compatibility and the matrix records that boundary.

## Suggested Next

- Execute Step 7.2: publish representative scalar compare result/use authority.

## Watchouts

- Own only one ordinary scalar integer `LirCmpOp` with native predicate/type
  facts, a `fresh_value` result, and the exact result ID preserved into its
  immediate same-function normalization or use.
- Include a directly coupled cast use only when production lowering cannot
  avoid it; treat it as a consumer and do not claim the cast producer anew.
- Accept misleading display after native authority is proven; reject invalid
  or duplicate results, unknown or cross-function uses, invalid predicates,
  and type conflicts.
- Keep select, abs, every other Step-7 row, pointer/object, aggregate/vector,
  CFG/terminators, parameters, calls, inline assembly, and new-BIR work outside
  Step 7.2.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.1 cast producer,
  verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- The supervisor clean-stashed full regression guard passed:
  `test_before.log` passed 3033/3033, `test_after.log` passed 3033/3033, and
  the monotonic delta was passed=0 and failed=0 with no new failures.
- `git diff --check` passed for the complete Step-7.1 slice.
