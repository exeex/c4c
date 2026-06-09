Status: Active
Source Idea Path: ideas/open/145_current_block_join_fact_routing_split.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Reusable Join-copy Source Facts

# Current Packet

## Just Finished

Step 2 moved the reusable current-block join-copy source status, source fact
structs, query inputs, and `prepare_current_block_join_parallel_copy_source_facts`
from `prepared_lookups` ownership into `publication_plans` ownership.
The reusable value-home/out-of-SSA source predicates moved with the source fact
query. `PreparedCurrentBlockJoinParallelCopyInstructionRouting` and
`prepare_current_block_join_parallel_copy_instruction_routing` remain in
`prepared_lookups` as the existing instruction-routing convenience.

AST-backed checks confirmed the source-fact producer definition is now in
`src/backend/prealloc/publication_plans.cpp`, while the routing wrapper
definition remains in `src/backend/prealloc/prepared_lookups.cpp`.

## Suggested Next

Proceed with the next packet by making the remaining current-block instruction
routing convenience target-local in AArch64, or by doing the plan-owner-selected
Step 3 packet if lifecycle state chooses a narrower intermediate slice.

## Watchouts

- `source_is_source_value` still carries existing shared fact naming that is
  close to AArch64 skip policy; review before exposing stronger shared policy
  names.
- `prepared_lookups.hpp` still declares the routing wrapper for compatibility;
  moving that wrapper should stay separate from this completed source-fact move.
- Supervisor escalated to full CTest because shared publication ownership
  changed.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. `ctest` reported 179/179 backend tests passed.
Proof log: `test_after.log`.

Supervisor follow-up:

```sh
ctest --test-dir build -j --output-on-failure
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: full CTest passed, 3427/3427 tests. Regression guard passed with
179/179 backend tests before and after, no new failures.
