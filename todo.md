Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Metadata Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 469: implemented the bounded prepared
branch-stack-load authority metadata surface and focused coverage.

Implemented surface:

- Added branch stack-load role names for `condition`, `lhs`, and `rhs`.
- Added branch stack-load policy names with the selected
  `load_from_stack_slot` route.
- Added pointer-status classification with `not_pointer`, `proven`, and
  `unknown`.
- Added `PreparedBranchStackLoadAuthorityStatus` fail-closed statuses for
  missing names/branch condition/terminator, unsupported terminator or branch
  condition, condition/target mismatch, missing or unsupported branch values,
  missing/mismatched homes, missing/mismatched frame slots,
  missing/mismatched stack objects, missing policy, missing freshness, missing
  clobber safety, and unknown pointer status.
- Added `plan_prepared_branch_stack_load_authority` and
  `prepared_branch_stack_load_authority_available`.

Focused coverage in `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
proves:

| Case | Result |
| --- | --- |
| Scalar stack-home branch condition with coherent branch identity, frame-slot/object relation, explicit `load_from_stack_slot`, freshness, and clobber-safety | accepted |
| Missing branch stack-load policy | `missing_policy` |
| Missing freshness | `missing_stack_freshness` |
| Missing clobber safety | `missing_stack_clobber_safety` |
| Mismatched value home | `home_value_mismatch` |
| Missing frame slot | `missing_frame_slot` |
| Mismatched frame slot object or offset | `frame_slot_mismatch` |
| Mismatched stack object | `stack_object_mismatch` |
| Pointer operand without proven pointer status | `pointer_status_unknown` |
| Pointer operand with proven status and coherent stack facts | accepted |
| Register-home branch value presented to stack-load authority | `unsupported_home` |

Boundaries preserved:

- No RV64 object emission changes.
- No weakening of `PreparedFusedPointerBranchPublication`.
- No raw BIR, testcase-name, stack-slot-number, block-order, or dump-shape
  inference.
- No pointer-value/provenance repair for `%t7`.
- No select-result/block-entry stack-destination repair for `%t22/%t23`.
- No expectation, allowlist, unsupported-marker, baseline, or pass/fail
  accounting changes.

Artifacts:

- `build/agent_state/469_step3_branch_stack_load_authority_metadata/summary.md`
- `test_after.log`

## Suggested Next

Execute Step 4 from `plan.md`: residual disposition and close-readiness review.

Suggested Step 4 packet:

- Owned files:
  `todo.md`,
  `test_after.log`,
  `build/agent_state/469_step4_residual_disposition/**`.
- Re-check the representative `930930-1` branch stack-home rows against the
  new metadata surface, classifying whether each row is now available,
  unavailable because producer freshness/clobber inputs are still absent, or
  owned by pointer-value/provenance or select-result stack-destination work.
- Decide whether idea 469 is close-ready as a metadata-surface slice, should
  remain active for a collector/population/printer packet, or should route a
  later RV64 consumer only after available records are actually populated.
- Proof command:

```sh
git diff --check
```

## Watchouts

- The implemented Step 3 surface is a planner/contract surface with focused
  tests. It does not yet collect/populate records from full prepared modules or
  print them in prepared dumps.
- Available records now require an explicit `PreparedFrameSlot` tying the
  value home slot id to the stack object id with matching offset, size, and
  alignment.
- A later RV64 consumer must require an available authority record; it must not
  consume stack homes directly.
- Pointer operands still require explicit proven pointer status. Unknown
  pointer-value memory and external-formal ambiguity remain fail-closed.
- Do not weaken existing GPR-compatible branch publication predicates.
- Keep `%t7` pointer-value/provenance and `%t22/%t23` select-result
  stack-destination work separate unless plan-owner selects a distinct route.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
