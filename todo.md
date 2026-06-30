Status: Active
Source Idea Path: ideas/open/452_select_edge_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Rematerialization Packet

# Current Packet

## Just Finished

Completed Step 3 by validating/routing the first rematerialization packet.
Supporting artifact:
`build/agent_state/452_step3_select_edge_rematerialization/summary.md`.

No implementation files or tests were changed. The selected
register/immediate select-edge compare rematerialization behavior is already
present in the current tree:

| Coverage / route | Existing proof |
| --- | --- |
| Simple prepared edge compare rematerialization | `materializes_published_prepared_join_transfer_select_edge_compare_source_object` proves a `source_producer=binary` compare is rematerialized into the selected destination register. |
| Dependent register/immediate producer chain | `materializes_published_prepared_join_transfer_select_dependent_edge_compare_source_object` proves register/immediate dependency producers are emitted before the edge compare. |
| Fail-closed boundaries | `rejects_published_prepared_join_transfer_select_ambiguous_publications_object` rejects non-select carrier and stack operand source shapes. |

Fresh `20010329-1` probing still fails at
`unsupported_move_bundle_target_shape`, which is expected: `%t18 -> %t22`
depends on `%t17` with a stack-slot pointer home and remains rejected from the
first register/immediate packet.

## Suggested Next

Step 4 should perform residual disposition and close-readiness review. The
expected recommendation is to close idea 452 for the register/immediate
source-producer rematerialization route and route the remaining `%t18/%t17`
stack-slot pointer dependency to idea 451 or a narrower follow-up.

## Watchouts

- Do not copy `%t18` on the predecessor edge unless edge availability is
  proven.
- Do not infer source-producer dependencies from raw BIR shape, block order,
  filenames, function names, or one prepared dump layout.
- Keep stack-home branch operand materialization routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md` unless Step 2
  explicitly accepts a narrow overlap as part of edge rematerialization.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Keep `20000622-1` out of the first packet while its first owner is
  instruction-side lowering.
- Do not treat a successor-defined compare result's register home as
  predecessor-edge availability.
- Do not let the later `%t32/%t46` register-compatible candidates hide the
  first `%t18/%t17` stack-slot dependency decision.
- Do not admit stack-slot pointer dependencies in the first implementation
  packet; `%t18 -> %t22` remains the fail-closed boundary case.
- Step 3 must require prepared source-producer identity and cannot discover
  compare/cast dependencies from raw BIR alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
