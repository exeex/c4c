# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove a representative non-call scalar result/use chain

## Just Finished

- Completed Plan Step 5 for the direct void fixed scalar SSA argument.
- Reused the common `LirOperand` call-argument carrier and preserved the exact
  CC-LOAD-1 selected-global scalar-load `LirValueId` when the direct fixed
  integer parameter keeps the same LLVM representation.
- Added authority-first exact verification for the structured SSA row: exact
  type refs, no result or extension, and a valid known value ID owned by the
  current function. Complete native packets ignore misleading text mirrors.
- Added focused positive and malformed coverage for exact source-ID reuse,
  missing/wrong/invalid/unknown/cross-function authority, and type, signature,
  count, and extension conflicts. The selected-global load contract is
  unchanged.

## Suggested Next

- Execute Plan Step 6: prove a representative non-call scalar result/use chain.

## Watchouts

- Own only the focused two-operation integer `LirBinOp` chain selected by the
  Step-1 matrix and probe.
- Reuse the owning value allocator and common `LirOperand`: allocate
  authoritative result IDs and preserve the exact first result ID into the
  later ordinary use in the same function.
- Preserve native opcode and type facts; do not derive either from rendering.
- Accept misleading display after native authority is proven; reject invalid
  or duplicate result IDs and unknown or cross-function uses.
- Keep CFG/terminators, parameters, pointer/object identity, aggregate/vector,
  inline assembly, calls, and new-BIR work outside Step 6.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-5 producer,
  verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- The supervisor-owned clean-stashed full regression guard passed:
  `test_before.log` passed 3033/3033, `test_after.log` passed 3033/3033, and
  the monotonic delta was passed=0 and failed=0 with no new failures.
- `git diff --check` passed for the complete Step-5 slice.
