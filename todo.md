Status: Active
Source Idea Path: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Freshness Clobber-Safety Producer

# Current Packet

## Just Finished

Completed Step 3 as a route/blocker packet for idea 471. No producer
implementation is selected because current prepared inputs do not prove a
durable branch-site current-value source or no-clobber safety for scalar
condition stack slots.

Checked implementation surfaces:

| Surface | Finding |
| --- | --- |
| `PreparedBranchStackLoadAuthorityInputs` | Carrier can express `load_from_stack_slot`, `stack_slot_fresh_at_branch`, and `stack_slot_clobber_safe_at_branch`. |
| `plan_prepared_branch_stack_load_authority` | Status machine already enforces `missing_policy`, `missing_stack_freshness`, and `missing_stack_clobber_safety`. |
| `collect_prepared_branch_stack_load_authorities` | Real records still pass `policy=none` and have no source for freshness or clobber-safety booleans. |
| Representative `%t23` row | Has branch/value/home/frame-slot/object identity, but lacks a producer-owned fact that slot `#21` contains current `%t23` at `f.logic.end.14` and remains unclobbered. |

Exact blocker:

| Missing fact | Why current inputs are insufficient |
| --- | --- |
| Branch-site current-value proof | BIR computes `%t23` near the branch, but no prepared fact says stack slot `#21` contains that current value at the load site. |
| Path/dominance validity | No prepared certificate ties the value source to all paths reaching the branch site. |
| Stack-write exclusion | No prepared summary proves no intervening write to the same frame slot/object. |
| Call/helper/inline-asm safety | Existing call/helper facts are not joined into branch-site slot clobber-safety proof. |
| Publication/move-bundle safety | Existing publication and move-bundle rows are not joined into a same-slot non-clobber certificate. |

Preserved fail-closed boundaries: `%t22` select-result stack destination,
`%t1` / `%t7` pointer-provenance rows, and `%t2` / `%t8`
`unsupported_terminator` rows remain separate owners. No RV64 target lowering
or raw-shape inference was introduced.

Artifact:
`build/agent_state/471_step3_freshness_clobber_producer/blocker.md`.

## Suggested Next

Route a new producer/prepared metadata packet:
`Define/produce branch-site stack-slot current-value and no-clobber certificate`.

Suggested scope:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/<next_step>_branch_site_slot_current_value_clobber/**`

Implement only if the producer can explicitly prove current-value source,
path/dominance validity, stack-write exclusion, call/helper effect safety, and
publication/move-bundle non-clobber for the exact branch slot. Otherwise keep
branch-stack-load records unavailable.

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not set freshness/clobber-safety from raw BIR adjacency, stack homes, or
  object identity alone.
- Step 3 must block if current-value source, path validity, stack-write
  summary, call/helper effects, or move/publication clobber facts are missing.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer freshness or clobber safety from stack homes, offsets, object
  ids, raw BIR, block labels, function names, testcase names, or dump order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` reports 100% tests passed, 0 tests failed
out of 327, followed by `git diff --check`.
