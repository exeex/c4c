Status: Active
Source Idea Path: ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Certificate Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 472. The contract is bounded to scalar
branch condition stack slots represented by `f.logic.end.14` `%t23`, slot
`#21`, and requires explicit producer-owned current-value plus no-clobber
certificates before any later branch-stack-load availability can be set.

Positive contract:

| Fact group | Required facts |
| --- | --- |
| Branch-site identity | Function, branch block, condition role/value, terminator kind, and true/false targets match the prepared branch condition row. |
| Stack-slot identity | Value id/name/type, home, frame slot id, stack object id, offset, size, and alignment match the branch-stack-load row. |
| Source identity | Producer names the current scalar value expected in the slot and ties it to the branch condition value. |
| Exact frame-slot materialization/write | Producer proves the source was written/materialized into the exact frame slot/object. |
| Path/dominance validity | Producer proves the materialization reaches the branch site on every authorized path, or records explicit path scope. |
| Same-slot write exclusion | Producer proves no intervening store, stack write, frame-slot publication, stack-destination move, or parallel copy overwrites the slot. |
| Call/helper/inline-asm effects | Producer proves no intervening call/helper/inline asm can invalidate the slot; unknown effects fail closed. |
| Publication/move-bundle/parallel-copy non-clobber | Producer proves prepared publications, move bundles, and parallel copies do not target or invalidate the slot. |

Rejected/fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Raw BIR adjacency, value names, function names, block labels, testcase shape, or dump order | Reject as raw-shape inference. |
| Stack home/object identity only | Reject as identity evidence, not materialization/current-value proof. |
| Missing source identity or exact frame-slot materialization | Fail closed; no certificate. |
| Path/dominance ambiguity | Fail closed unless explicitly path-scoped and accepted by the later consumer. |
| Same-slot stack write, publication, move bundle, or parallel copy ambiguity | Fail closed as clobber-unsafe. |
| Unknown call/helper/inline-asm effects | Fail closed as clobber-unsafe. |
| `%t22` select-result stack destination | Separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate owner. |
| `%t2` / `%t8` `unsupported_terminator` rows | Separate branch-site relationship owner. |

Artifact:
`build/agent_state/472_step2_current_value_no_clobber_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Certificate Producer.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/472_step3_current_value_no_clobber_producer/**`

Optional prepared printer files only if exposed certificate records/statuses
are added.

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Step 3 must route/block if current producer facts cannot explicitly prove
  source identity, exact frame-slot materialization, path validity, same-slot
  write exclusion, call/helper effects, and publication/move non-clobber.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer current value or no-clobber safety from stack homes, offsets,
  object ids, raw BIR, block labels, function names, testcase names, or dump
  order.
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
