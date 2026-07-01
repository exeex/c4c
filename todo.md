Status: Active
Source Idea Path: ideas/open/505_select_publication_stack_home_intent_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Stack-Home Intent Construction

# Current Packet

## Just Finished

Idea 504 closed after Step 4 residual disposition. It completed the
select-publication evidence/authority packet and did not implement RV64
materialization.

Step 3 classification found no coherent final select-publication RV64 consumer
ready from the three rows:

- `src/builtin-constant.c` is real select-publication evidence but needs
  stack-source intent support.
- `src/pr58726.c` is real select-publication evidence but needs
  stack-destination intent support.
- `src/pr37924.c` is not select-publication after the new evidence; it is a
  generic out-of-SSA `phi_join_immediate_materialization` row recorded in
  idea 506.

Lifecycle activated idea 505 for the select-publication stack-home intent
support prerequisite.

## Suggested Next

Execute Step 1 by inspecting `EdgePublicationMoveIntent` construction for the
two select-publication stack-home rows. Record helper surfaces, missing intent
fields, representative rows, and whether implementation can proceed.

## Watchouts

- Do not implement RV64 select-publication materialization under idea 505.
- Do not fold generic immediate materialization from `src/pr37924.c` into this
  select-publication intent work.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
