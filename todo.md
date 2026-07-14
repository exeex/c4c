# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.2
Current Step Title: Publish representative scalar compare result/use authority (complete)

## Just Finished

- Completed Plan Step 7.2 for the representative ordinary scalar integer
  `LirCmpOp` result/use chain.
- Allocated the integer comparison result through `fresh_value`, preserved
  unchanged compared operands, and passed the exact result `LirOperand` into
  the existing immediate i1-to-i32 normalization cast.
- Preserved native Slt and exact i32 compared-type authority. The directly
  coupled cast remains a monostate-result consumer and does not broaden the
  Step-7.1 producer claim.
- Added focused exact-ID and misleading-display positives plus invalid/
  duplicate result, unknown/cross-function use, invalid predicate, and missing
  or conflicting type rejections. Other comparison producers remain
  compatibility and the matrix records that boundary.

## Suggested Next

- Select the next bounded Plan Step 7 ordinary producer row from the updated
  matrix; select and abs remain unclaimed candidates.

## Watchouts

- Do not generalize Step 7.2 to float, pointer, vector, logical-helper, builtin,
  vaarg, or statement comparisons; each remains unclaimed.
- Select and abs may reuse the common allocator/operand mechanism but still
  require separate opcode/type contracts and focused proof.
- Preserve closed Step-3 through Step-7.2 rows and idea-741 regression
  neighbors; keep pointer/object, aggregate/vector, CFG/parameters, calls,
  inline assembly, ABI, and BIR outside the next packet.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.2 comparison
  producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- The supervisor clean-stashed full regression guard passed:
  `test_before.log` passed 3033/3033, `test_after.log` passed 3033/3033, and
  the monotonic delta was passed=0 and failed=0 with no new failures.
- `git diff --check` passed for the complete Step-7.2 slice.
