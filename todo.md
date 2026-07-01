Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Semantic Admission From Global/Static GEP Authority

# Current Packet

## Just Finished

Lifecycle retained active idea 500 after Step 3 because the source idea owns
final semantic global/static GEP admission. Step 3 published the lower
`global_static_gep_authority` certificate surface; Step 4 should consume those
records to publish final semantic admission.

## Suggested Next

Implement Step 4 by publishing final semantic global/static GEP admission
records only from matching available `global_static_gep_authority` records.

## Watchouts

- Do not reconstruct authority from prepared object data, final homes,
  relocations, raw testcase shape, names, labels, object files, or target
  behavior.
- Keep pointer/string, runtime/string intrinsic, aggregate/member,
  raw-shape-only, target-only, and coordinate-confusion rows fail-closed unless
  a matching available authority and consumer-specific prerequisite exists.
- This remains BIR semantic producer work, not RV64/MIR lowering or object
  emission.
- Preserve the rejected `test_baseline.new.log` and untracked review artifacts.

## Proof

Lifecycle extension proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
