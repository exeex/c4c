# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove a representative non-call scalar result/use chain (complete)

## Just Finished

- Completed Plan Step 6 for the focused two-operation ordinary scalar integer
  `LirBinOp` result/use chain.
- Added an operand-returning binary-expression seam: normalized scalar integer
  arithmetic allocates results with `fresh_value`, returns the common
  `LirOperand`, and preserves input authority only across representation-
  preserving coercion.
- Preserved exact native Add/Mul and i32 facts and proved that the later Mul lhs
  carries the exact earlier Add result ID. Other binary branches retain
  monostate compatibility.
- Added focused misleading-display acceptance and rejection for invalid or
  duplicate results, unknown or cross-function uses, invalid opcode, and
  missing type authority. Updated the matrix with exact shared versus distinct
  neighboring row classifications.

## Suggested Next

- Execute the first coherent Plan Step 7 generic ordinary producer group
  justified by the updated matrix.

## Watchouts

- Group Step-7 rows only when result allocation, operand propagation, type
  authority, and verifier rules are identical; use one bounded executor packet
  per group.
- Scalar compare, cast, select, and abs can reuse the common allocator/operand
  mechanism, but each still needs its opcode/predicate/type contract checked
  before publication.
- Keep aggregate/vector type/index/mask semantics and pointer/object, CFG,
  parameter, inline-asm, call, ABI, and BIR families separate.
- Preserve the closed Step-3 through Step-6 rows and all four idea-741
  regression neighbors while extending production coverage.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-6 producer seam
  and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- The supervisor clean-stashed full regression guard passed:
  `test_before.log` passed 3033/3033, `test_after.log` passed 3033/3033, and
  the monotonic delta was passed=0 and failed=0 with no new failures.
- `git diff --check` passed for the complete Step-6 slice.
