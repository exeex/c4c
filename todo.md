Status: Active
Source Idea Path: ideas/open/450_select_result_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Select-Result Packet

# Current Packet

## Just Finished

Completed Step 3 by routing the first register-home select-result branch
packet instead of implementing an unsound consumer-only shortcut. Supporting
artifact:
`build/agent_state/450_step3_select_result_branch_publication/blocker.md`.

Fresh `20010329-1` probes still stop at
`unsupported_move_bundle_target_shape`. The first `%t22` row has the expected
prepared select-result branch shape and register homes, but one incoming
select edge is `%t18 -> %t22`. `%t18` is a successor/join-block compare:
`%t18 = bir.ule ptr %t15, %t17`, where `%t17` is produced by `inttoptr` and
currently has a stack-slot pointer home. The existing RV64 select-edge source
producer rule can rematerialize only supported register/immediate compare
operands. A plain register-copy fallback from `%t18` would be wrong because
the out-of-SSA copy executes on the predecessor edge, before `%t18` is defined
in the successor block.

No implementation files or tests were changed.

## Suggested Next

Step 4 should perform residual disposition and close-readiness review for idea
450, with the current route decision recorded: the next executable work is not
a pure register-home branch consumer. It needs either a focused prepared
source-producer dependency contract for select-edge rematerialization, or a
split plan that owns pointer cast / compare dependency rematerialization before
the register-home select-result branch consumer can be made sound.

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
- Do not consume non-zero compare constants or predicates outside `Eq`/`Ne`
  in the first packet.
- Do not infer select-result publication from raw select shape,
  `root_is_select=yes`, raw `ne i32 <select>, 0`, block order, filenames,
  function names, or one dump layout.
- Keep missing or incoherent select-result publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 route/blocker validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
