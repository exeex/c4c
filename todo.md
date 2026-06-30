Status: Active
Source Idea Path: ideas/open/450_select_result_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 450. Supporting artifact:
`build/agent_state/450_step4_residual_disposition/disposition.md`.

Fresh probes confirm the residual classification:

| Row | Current object result | First owner |
| --- | --- | --- |
| `20010329-1` | `unsupported_move_bundle_target_shape` | Select-edge source-producer dependency rematerialization for predecessor-edge out-of-SSA move bundles. |
| `20000622-1` | `unsupported_instruction_fragment` | Instruction-side lowering before select-result branch consumption is first. |

`20010329-1 main.logic.end.14` still has complete select-result branch
candidate facts for `%t22/%t23`, including `root_is_select=yes`,
`source_producer=select_materialization`, a prepared `ne i32 %t22, 0` branch
condition, GPR homes, and available register-destination block-entry
publications. The blocker remains the incoming `%t18 -> %t22`: `%t18` is a
successor/join-block compare over `%t15` and `%t17`, and `%t17` has a
stack-slot pointer home. The predecessor-edge move site therefore needs
explicit prepared rematerialization of the compare/cast dependency chain; a
plain copy from `%t18` would be unsound.

## Suggested Next

Plan-owner should split or activate a durable follow-up for prepared
select-edge source-producer dependency rematerialization. Idea 450 should not
continue with another direct register-home branch consumer packet until that
authority exists. If the plan owner prefers lifecycle closure, close 450 as
blocked by the split and preserve current fail-closed behavior for
`20010329-1`.

## Watchouts

- Keep this plan limited to select-result branch publication and move-bundle
  target materialization.
- Do not reopen pointer `eq/ne` or unsigned relational branch publication;
  those routes are closed by ideas 441 and 449.
- Do not fold stack-home branch operand materialization into this plan; that
  belongs to `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep `20000622-1` out of the first implementation packet while its current
  first object-route owner is `unsupported_instruction_fragment`.
- Keep `930930-1` stack-slot select-result publication out of this packet;
  its current prepared rows are not register-destination consumer facts.
- Do not accept `root_is_select=yes` alone as authority; Step 3 must require
  the full prepared select-chain, branch-condition, homes, and publication
  target contract.
- Do not copy from a successor-block compare result on a predecessor edge just
  because the compare result has a register home; the source must be available
  at the edge or explicitly rematerialized by prepared source-producer facts.
- Do not fold pointer cast / compare dependency rematerialization into this
  packet without a focused prepared authority contract.
- Keep the rematerialization follow-up separate from stack-home branch operand
  materialization unless that plan explicitly owns the overlap.
- Do not consume non-zero compare constants or predicates outside `Eq`/`Ne`
  in the first packet.
- Do not infer select-result publication from raw select shape,
  `root_is_select=yes`, raw `ne i32 <select>, 0`, block order, filenames,
  function names, or one dump layout.
- Keep missing or incoherent select-result publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 4 residual-disposition validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
