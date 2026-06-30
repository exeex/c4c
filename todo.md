Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate And Expose Branch Stack-Load Authority Records

# Current Packet

## Just Finished

Completed Step 4 close-readiness review for idea 469. The Step 3 planner and
contract surface is complete, but the source idea is not fully close-ready
because real prepared modules still lack collected/populated/printed
`PreparedBranchStackLoadAuthority` records.

Remaining in-scope packet:

| Area | Required next work |
| --- | --- |
| Record collection | Collect branch-stack-load authority records from prepared control-flow, value-home, frame-slot, and stack-object facts. |
| Record population | Populate available or unavailable records for representative branch stack-home rows. |
| Visibility | Print or otherwise expose records through durable prepared evidence/probe surfaces. |
| Unavailable statuses | Preserve missing policy, missing freshness, missing clobber-safety, pointer-status unknown, and select-result stack-destination boundaries. |
| RV64 | Do not resume target consumption until populated available records exist. |

Representative rows to classify:

- `f.block_1` condition `%t2`: scalar stack-home condition; should remain
  unavailable until branch-site load policy, freshness, and clobber-safety
  records exist.
- `f.block_1` lhs `%t1`: pointer stack-home lhs; additionally requires
  explicit proven pointer status.
- `f.block_4` condition `%t8`: scalar condition row; `%t7` remains a separate
  pointer-value/provenance boundary.
- `f.logic.end.14` condition `%t23`: scalar condition row; `%t22` remains a
  separate select-result/block-entry stack-destination boundary.

## Suggested Next

Execute Step 5 from `plan.md`: implement or route branch-stack-load authority
record collection/population/printer visibility. Keep it producer/prepared-only
and do not edit RV64 object emission.

Suggested artifact directory:
`build/agent_state/469_step5_branch_stack_load_record_population/`.

Likely owned files for an executor packet:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- prepared printer files if dump exposure is needed
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp` if printer coverage is
  added
- `todo.md`
- `test_after.log`
- `build/agent_state/469_step5_branch_stack_load_record_population/**`

## Watchouts

- Do not edit RV64 object emission or implement target branch loads in this
  packet.
- Do not infer load authority from stack slot ids, offsets, object ids, block
  labels, or prepared dump spelling.
- Do not treat pointer operands as proven unless pointer status is explicit.
- Keep `%t7` pointer-value/provenance and `%t22` select-result stack-destination
  boundaries separate.
- Do not weaken GPR-compatible branch predicates.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, or `review/`.

## Proof

Lifecycle repair proof:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
