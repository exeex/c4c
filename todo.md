Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 2 published explicit parallel-copy bundle execution-site authority for
predecessor-terminator, successor-entry, and critical-edge cases, and added
prepared dump plus backend coverage for the new critical-edge bundle contract.

## Suggested Next

Extend the published bundle contract with the next missing downstream-facing
fact: make consumer use of the new critical-edge execution-site authority
truthful instead of leaving regalloc and later phases to treat every bundle as
predecessor-terminator executable.

## Watchouts

- `src/backend/prealloc/regalloc.cpp` still schedules bundle move resolution
  at the predecessor block, so newly published `execution_site=critical_edge`
  is truthful contract data but not yet consumed as an execution constraint.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed; proof log written to `test_after.log`.
