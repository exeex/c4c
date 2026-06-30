Status: Active
Source Idea Path: ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Certificate Producer

# Current Packet

## Just Finished

Completed Step 3 as a route/blocker packet for idea 472. No certificate
producer was implemented because current prepared inputs still cannot prove
exact frame-slot materialization/current-value or no-clobber safety for
`f.logic.end.14` `%t23`, slot `#21`.

Checked surfaces:

| Surface | Finding |
| --- | --- |
| `PreparedBranchStackLoadAuthorityInputs` | Downstream carrier can hold policy/freshness/clobber booleans, but not the certificate source itself. |
| `plan_prepared_branch_stack_load_authority` | Correctly rejects records without policy, freshness, and clobber safety. |
| `collect_prepared_branch_stack_load_authorities` | Reaches branch condition, terminator, value home, frame slot, and stack object; it has no materialization/current-value or no-clobber certificate input. |
| Existing prepared evidence | Exposes calls, store-source rows, publications, move bundles, and parallel copies, but no joined certificate for slot `#21`. |

Exact blocker:

| Missing fact | Current status |
| --- | --- |
| Exact frame-slot materialization/current-value | No prepared row states current `%t23` was written/materialized into slot `#21` before the branch. |
| Path/dominance validity | No certificate connects any source materialization to all paths reaching `logic.end.14`. |
| Same-slot write exclusion | No producer summary proves slot `#21` has no intervening stack write/publication/move/parallel-copy clobber. |
| Call/helper/inline-asm effects | No effect certificate is tied to slot `#21`. |
| Publication/move-bundle/parallel-copy non-clobber | Existing rows are not joined into a slot `#21` non-clobber proof. |

Preserved fail-closed boundaries: `%t22` remains select-result stack
destination/block-entry publication work; `%t1` / `%t7` remain
pointer/provenance boundaries; `%t2` / `%t8` remain `unsupported_terminator`
branch-site relationship work. No RV64 lowering or raw-shape inference was
introduced.

Artifact:
`build/agent_state/472_step3_current_value_no_clobber_producer/blocker.md`.

## Suggested Next

Route a lower-level prepared producer initiative for branch-site stack-slot
write/current-value and no-clobber certificate sources.

Required future facts:

- source value and exact frame-slot materialization/write;
- path/dominance relationship to the branch site;
- same-slot write exclusion;
- call/helper/inline-asm effect safety;
- publication, move-bundle, and parallel-copy non-clobber safety;
- positive/fail-closed tests before populating branch-stack-load authority.

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

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` reports 100% tests passed, 0 tests failed
out of 327, followed by `git diff --check`.
