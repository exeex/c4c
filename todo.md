Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 469: residual disposition and close-readiness
review after commit `2a60f4458` added the prepared branch-stack-load authority
planner surface.

Decision: idea 469 is complete for the bounded planner/contract surface, but
not close-ready as fully published prepared metadata. The precise remaining
in-scope packet is collector/population/printer exposure for
`PreparedBranchStackLoadAuthority` records, including unavailable statuses for
the representative rows. RV64 branch-load consumption must not resume until
available records are actually populated.

Representative residual table:

| Row | Current disposition after Step 3 | First remaining owner |
| --- | --- | --- |
| `f.block_1` condition `%t2` | Structurally covered by the new planner for scalar stack-home branch conditions, but no full-module collector/printer currently emits a record; no prepared fact yet supplies branch-site `load_from_stack_slot`, freshness, or clobber-safety for the real row. | Idea 469 follow-up: collect/populate/print branch-stack-load authority records, initially unavailable for missing policy/freshness/clobber. |
| `f.block_1` lhs `%t1` | Planner can represent a proven pointer stack-home lhs, but the real row lacks populated branch-stack-load policy/freshness/clobber facts and still needs explicit proven pointer status. | Idea 469 population first, then pointer/provenance authority boundary for external-formal/frame-slot provenance if pointer status remains unknown. |
| `f.block_1` rhs `%p.reg2` | Register-home rhs; not a branch-stack-load row. | Existing GPR branch route, out of scope for 469. |
| `f.block_4` condition `%t8` | Scalar stack-home condition would be representable by the planner, but no record is populated and the paired lhs `%t7` remains pointer/provenance blocked. | 469 population for condition status; pointer-value/provenance owner for `%t7`. |
| `f.block_4` lhs `%t7` | Stack-home pointer lhs remains unavailable; `%t7` derives from pointer-value memory with unknown-compatible provenance/layout evidence. | Pointer-value/provenance authority, not 469 alone. |
| `f.logic.end.14` condition `%t23` | Scalar stack-home condition would be representable by the planner, but the branch depends on `%t22`. | 469 population for condition status; select-result/block-entry stack-destination owner for `%t22`. |
| `f.logic.end.14` lhs `%t22` | Select-result stack destination remains an adjacent owner; do not accept through branch stack-load metadata. | Select-result/block-entry stack-destination route. |
| `f.block_7` pointer eq/ne branch | GPR-compatible and already out of scope. | Closed by earlier GPR branch work. |

Lifecycle recommendation: keep/split one exact producer-prepared follow-up for
branch-stack-load authority record collection and printer/probe visibility.
After that packet, residual disposition can decide whether an RV64 consumer is
ready or whether freshness/clobber or pointer-status producers remain missing.

Artifact:
`build/agent_state/469_step4_residual_disposition/disposition.md`.

## Suggested Next

Select a focused collector/population/printer packet for idea 469, or have the
plan owner split it if the supervisor wants the planner surface committed as a
closed prerequisite first.

Suggested packet:

- Owned files:
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  prepared printer files if records need dump exposure,
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp`,
  `tests/backend/bir/backend_prepared_printer_test.cpp` if printer coverage is
  added,
  `todo.md`,
  `test_after.log`,
  `build/agent_state/469_step5_branch_stack_load_record_population/**`.
- Add collection/population for branch-stack-load authority records keyed by
  function, branch, role, value/home/frame-slot/object.
- Print/probe available and unavailable statuses so real `930930-1` rows can
  be classified without raw-shape inference.
- Preserve fail-closed status for missing policy, missing freshness, missing
  clobber-safety, unknown pointer status, and select-result stack destination.
- Do not edit RV64 object emission.

## Watchouts

- The Step 3 planner accepts only explicit caller-provided facts. The current
  prepared pipeline does not yet populate those facts for representative
  modules.
- Real `930930-1` rows should remain unavailable until record population and
  freshness/clobber facts exist.
- Do not infer load authority from stack slot ids, offsets, object ids, block
  labels, or prepared dump spelling.
- Do not treat pointer operands as proven unless pointer status is explicit.
- Keep `%t7` pointer-value/provenance and `%t22` select-result stack-destination
  boundaries separate.
- Do not modify implementation files, tests, `test_after.log`,
  `test_before.log`, baseline logs, or `review/` during this Step 4 packet.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
