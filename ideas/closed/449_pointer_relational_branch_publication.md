# Pointer Relational Branch Publication

Status: Closed
Type: Terminator/select publication idea
Parent: `ideas/closed/441_terminator_select_publication_authority.md`
Source Evidence: `build/agent_state/441_step4_residual_disposition/`
Close Evidence: `build/agent_state/449_step4_residual_disposition/`
Owning Layer: Prepared branch-condition publication before RV64 lowering

## Goal

Define and consume explicit authority for fused pointer relational branch
predicates such as `uge ptr` and `ult ptr`, without treating them as a small
extension of pointer equality/inequality publication.

## Completion Notes

Closed as complete for the selected unsigned pointer relational branch route.

- Step 2 defined the accepted policy for unsigned pointer relational predicates
  `Ult`, `Ule`, `Ugt`, and `Uge`.
- Step 3 implemented the prepared-fact-driven RV64 consumer for fused pointer
  relational branches with matching labels, GPR-compatible condition homes, and
  GPR-compatible named pointer operand homes.
- `20010329-1 main.entry`
  `branch_condition entry kind=fused_compare condition=%t8 compare=uge ptr %t5, %t7`
  no longer owns `unsupported_terminator_fragment`.
- Focused coverage preserves fail-closed behavior for signed pointer
  relational predicates, non-null pointer immediates, missing branch
  conditions, missing operand homes, mismatched labels, and stack-home
  operand/condition cases.

## Residual Disposition

No exact remaining pointer relational branch packet is currently justified
inside this source idea.

Durable follow-up owners:

- `ideas/open/450_select_result_branch_publication.md` owns select-result
  branch publication and move-bundle target materialization for
  `20010329-1` and similar `root_is_select=yes` rows.
- `ideas/open/451_stack_home_branch_operand_materialization.md` owns stack-home
  branch operand/condition materialization if branch consumers later support
  stack homes.
- `ideas/open/442_pointer_value_memory_provenance_publication.md` owns
  pointer-value/provenance publication before reconsidering `930930-1` pointer
  relational rows.

Still out of scope:

- pointer equality/inequality branch publication, closed by idea 441;
- select-result publication, stack-home expansion, pointer-value/provenance,
  local/global store-source publication, unsupported instruction/storage
  residuals;
- expectation rewrites, allowlists, pass/fail accounting, and accepting or
  modifying `test_baseline.new.log`.

## Close Validation

Supervisor-provided validation says the Step 4 backend proof and regression
guard passed before log roll-forward, with before and after both reporting
`327/327` passing tests.

Because `test_after.log` had already been rolled forward and was absent at
close review time, the plan-owner performed a read-only guard sanity check
against the rolled-forward canonical log:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
```

Result: PASS, with `327/327` passing tests.

`git diff --check` also passed.

## Reviewer Reject Signals

- Reject reopening this closed idea to infer pointer relational branch
  semantics from raw BIR compare/branch shape, block order, filenames,
  function names, testcase names, or one prepared dump layout.
- Reject treating signed pointer relational predicates, stack-home
  operand/condition cases, select-result branch publication, or
  pointer-value/provenance gaps as covered by this closed unsigned GPR-home
  route.
- Reject broadening the completed unsigned relational branch route into
  move-bundle materialization or unsupported instruction/storage repair.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as relational branch progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
