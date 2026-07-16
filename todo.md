# Current Packet

Status: Active
Source Idea Path: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Diagnose and repair the rejected post-commit baseline

## Just Finished

- Step 2 completed the selected direct-global, non-variadic, specified
  two-parameter argument-1 relation. `prepare_call_arg` now preserves the
  already-native `source_operand` and `LirTypeRef(out_llvm_ty)` only when
  argument 1 is an unconverted current-function `DirectScalar` SSA source.
  Existing owned-call construction carries them into `structured_args[1]` and
  `arg_type_refs[1]` without recovering either from presentation text.
- The native verifier now recognizes that exact direct two-parameter shape
  when argument 1 names a current-function `DirectScalar` definition, and
  rejects missing or incoherent argument/type/fixed-parameter-1 mirrors.
  Focused positive and malformed coverage verifies the native relation; no
  body-parameter authority tuple or role was added.

## Suggested Next

- Step 3 is an acceptance blocker: diagnose the full-suite candidate rejected
  after `f0fc85e4f` before treating the focused 1/1 result as sufficient.
  The candidate regressed from 3038/3038 to 3024/3038, with
  `frontend_hir_tests` segfaulting and 13 `llvm_gcc_c_torture` failures.
- Establish whether those failures are introduced by the 830 slice, repair
  only an in-scope cause if present, and rerun the comparable full baseline.
  Do not make another commit until the supervisor has proof that no new
  baseline problem remains relative to the accepted 3038/3038 baseline.
- Only then resume the focused completion record and 829 Step 2 return route;
  do not widen into parameter authority, Raw-BIR, generic calls, or other
  argument indices.

## Watchouts

- Keep 830 limited to the direct/non-variadic/specified argument-1 native
  relation. Do not touch Raw-BIR, 734, generic call arguments, or unrelated
  821/822 material. `args_str` and rendered operand/type strings remain
  compatibility mirrors, not sources of this relation.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
  The focused `frontend_lir_call_type_ref` subset passed (1/1); proof log:
  `test_after.log`.
- Rejected baseline gate: post-commit `test_baseline.new.log` for
  `f0fc85e4f` is not acceptance evidence (3024/3038 versus the accepted
  3038/3038 baseline). A fresh comparable full-suite result showing no new
  baseline problem is required before any further commit.
