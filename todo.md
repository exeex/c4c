Status: Active
Source Idea Path: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Freshness Clobber-Safety Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 471. The contract is bounded to scalar
condition branch-stack-load rows, with `%t23` at `f.logic.end.14` as the
representative, but Step 3 must first prove a durable current-value source and
no-clobber facts before setting any available record.

Positive contract:

| Fact group | Required facts |
| --- | --- |
| Branch identity | Function, branch block, prepared branch condition, BIR conditional branch terminator, condition value, and targets match. |
| Slot identity | Value id/name, home, frame slot, object id, offset, size, alignment, and type match. |
| Current-value source | Producer-owned fact states the slot contains the current branch condition value at the branch site. |
| Path validity | Source is valid on every authorized path to the branch site. |
| No clobber | No intervening stack write, call/helper/inline asm, publication, move bundle, or parallel copy can overwrite or invalidate the slot. |
| Policy integration | Only after the above may `load_from_stack_slot`, `fresh=true`, and `clobber_safe=true` be set. |

Rejected/fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Raw BIR adjacency | Rejected; `%t23` computed before the branch does not prove slot `#21` contains `%t23`. |
| Stack home/object only | Rejected as identity facts only, not freshness. |
| Missing current-value source or path proof | Fail closed as missing freshness. |
| Intervening store/call/helper/publication/move-bundle ambiguity | Fail closed as clobber-unsafe. |
| `%t22` select-result stack destination | Separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate owner. |
| `%t2` / `%t8` `unsupported_terminator` rows | Separate branch-site relationship prerequisite. |

Selected Step 3 packet: `Implement Or Route Scalar Condition Freshness
Producer`. Implement only if current prepared facts can prove a durable
current-value source and no-clobber safety without raw-shape inference;
otherwise route/block with the exact missing producer owner.

Artifact:
`build/agent_state/471_step2_freshness_clobber_contract/contract.md`.

## Suggested Next

Execute Step 3: Implement Or Route Freshness Clobber-Safety Producer.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/471_step3_freshness_clobber_producer/**`

Optional prepared printer files only if a new record/status is exposed.

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

Contract proof:

```sh
git diff --check
```

Result: passed.
