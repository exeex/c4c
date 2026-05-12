Status: Active
Source Idea Path: ideas/open/184_direct_call_signature_metadata_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory direct-call signature lowering

# Current Packet

## Just Finished

Lifecycle switched from closed audit idea 183 to open follow-up idea 184.
No executor packet has run for idea 184 yet.

## Suggested Next

Execute Step 1 from `plan.md`: inventory generated direct-call LIR construction,
LIR-to-BIR direct-call lowering, existing structured signature facts, and any
rendered-signature fallback points. This is a source-inspection packet; run
`git diff --check -- todo.md` after updating the inventory.

## Watchouts

- Do not add a new rendered signature parser branch.
- Do not let generated metadata-rich calls silently fall back to string
  parsing.
- Preserve explicit raw/no-metadata compatibility rather than deleting it as a
  side effect of the generated-path fence.
- Backend freeze closure remains owned by idea 188.

## Proof

Lifecycle activation only. Close-time regression guard for idea 183 used:

- `cmake --preset default`
- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Backend subset stayed at 109 passed, 0 failed before and after.
