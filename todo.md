Status: Active
Source Idea Path: ideas/open/507_rv64_select_publication_stack_home_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Final Admission Gate

# Current Packet

## Just Finished

Idea 505 closed after residual disposition. Step 2 added structured
select-publication stack-home intent support for:

- pointer/XLEN concrete stack-source to GPR destination intent
  (`src/builtin-constant.c` shape);
- direct GPR source to 1/2/4-byte concrete stack-destination intent
  (`src/pr58726.c` shape).

Final RV64 select-publication admission intentionally remains fail-closed.
Generic out-of-SSA immediate materialization for `src/pr37924.c` remains routed
to idea 506. The rejected `test_baseline.new.log` is preserved for
`string_authority_guard` diagnosis.

Lifecycle activated idea 507 for the final RV64 select-publication consumer
that consumes the structured stack-home intent fields.

## Suggested Next

Execute Step 1 by inspecting the final select-publication RV64 admission
predicate and emission path for the supported stack-home intent fields. Record
the exact predicate/helper surfaces and whether implementation can proceed
without new producer facts.

## Watchouts

- Do not fold generic immediate materialization from `src/pr37924.c` into idea
  507.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Keep stack-to-stack select-publication unsupported without a dedicated
  scratch/interleaving policy.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
