Status: Active
Source Idea Path: ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory global/type/layout lookup surfaces

# Current Packet

## Just Finished

Lifecycle close/switch completed for idea 184. The direct-call signature
metadata structured boundary was accepted as closed, and idea 185 is now the
active plan.

## Suggested Next

Execute Step 1 of `plan.md`: inventory `GlobalTypes`, `TypeDeclMap`, and
`BackendStructuredLayoutTable` construction and consumers, then record generated
metadata-rich routes versus raw/no-id compatibility routes here.

## Watchouts

- Idea 185 is a boundary audit plus selected generated-path repair; do not
  rewrite all legacy type declaration parsing in one pass.
- Preserve printer/final spelling and raw imported LIR compatibility unless a
  selected generated metadata-rich path proves it must fail closed.
- Backend freeze closure remains owned by idea 188.

## Proof

Close-time regression guard for idea 184 passed using canonical logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before 110 passed, 0 failed; after 110 passed, 0 failed.
