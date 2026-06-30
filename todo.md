Status: Active
Source Idea Path: ideas/open/451_stack_home_branch_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Stack-Home Branch Evidence

# Current Packet

## Just Finished

Activated idea 451 as the next concrete open lifecycle item. No implementation
files, tests, logs, closed archive files, or review artifacts were edited.

Selection rationale: 451 is a concrete RV64/prepared follow-up for stack-home
branch operand/condition materialization. Umbrella/policy ideas remain open,
and 442 remains parked behind unresolved external-linkage provenance
authority.

## Suggested Next

Execute Step 1 from `plan.md`: audit the `449_step4` residual evidence and
current prepared/object output for stack-home branch operand or condition rows.
Classify each representative row by stack object identity, offset, width,
freshness/clobber facts, pointer provenance status, and first owning layer.

Suggested artifact directory:
`build/agent_state/451_step1_stack_home_branch_evidence/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not weaken existing GPR-compatible branch publication predicates.
- Do not infer stack-home values, freshness, loads, operands, or conditions
  from raw BIR shape, stack-slot spelling, block order, filenames, function
  names, or one prepared dump layout.
- Keep pointer-value/provenance publication, select-result/move-bundle
  materialization, instruction/storage owners, and unrelated residuals separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle activation proof:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
