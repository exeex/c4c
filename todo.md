Status: Active
Source Idea Path: ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Source Fact Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 473. The source-fact contract is bounded to
scalar branch condition stack slots represented by `%t23`, slot `#21`, and
requires explicit materialization/current-value plus no-clobber source facts
before any downstream certificate or branch-stack-load authority can consume
them.

Positive contract:

| Fact group | Required facts |
| --- | --- |
| Branch-site identity | Function, branch block, condition role/value, terminator kind, and branch targets identify the consumer site. |
| Stack-slot identity | Value id/name/type, home kind, frame slot id, stack object id, offset, size, and alignment identify the slot. |
| Exact source value identity | Producer names the scalar source value intended to reside in the slot and ties it to the branch condition value. |
| Explicit materialization/write event | Producer names a concrete write/materialization event into the exact frame slot/object. |
| Path/dominance validity | Producer proves the materialization reaches the branch site on all relevant paths or records explicit path/predecessor scope. |
| Same-slot write exclusion | Producer proves no intervening store, stack write, publication, move bundle, or parallel copy writes the same slot/object. |
| Call/helper/inline-asm safety | Producer proves no intervening call/helper/inline asm or unknown effect can clobber the slot. |
| Publication/move-bundle/parallel-copy non-clobber | Producer proves prepared publications, move bundles, and parallel copies do not target or invalidate the slot. |
| Consumer boundary | Records are source facts only; they do not directly mark branch-stack-load authority available. |

Rejected/fail-closed cases:

| Shape | Disposition |
| --- | --- |
| Raw BIR adjacency, stack homes/storage/objects, value names, function names, block labels, testcase shape, or dump order | Reject as raw-shape inference. |
| Missing source value or explicit materialization/write event | Fail closed as missing source/materialization. |
| Source/destination/type/width/alignment mismatch | Fail closed as materialization mismatch. |
| Path/dominance unknown | Fail closed unless explicitly path-scoped for later consumption. |
| Same-slot write after materialization | Fail closed as clobbered. |
| Unknown call/helper/inline-asm effect | Fail closed as clobber-unsafe. |
| Publication/move-bundle/parallel-copy effects not analyzed | Fail closed as clobber-ambiguous. |
| `%t22` select-result stack destination | Separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Separate owner. |
| `%t2` / `%t8` `unsupported_terminator` rows | Separate branch-site relationship owner. |

Artifact:
`build/agent_state/473_step2_materialization_no_clobber_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: Implement Or Route Source Fact Producer.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- optional prepared printer files only if new records/statuses are exposed
- `todo.md`
- `test_after.log`
- `build/agent_state/473_step3_materialization_no_clobber_source/**`

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Step 3 must route/block if current prepared facts cannot explicitly prove
  exact materialization/write, path validity, same-slot write exclusion,
  call/helper safety, and publication/move non-clobber.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes, offsets, object ids, raw BIR,
  value names, block labels, function names, testcase names, or dump order.
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
