Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed representative direct-global return/select-chain
evidence and recorded residual disposition under
`build/agent_state/440_step4_residual_disposition/`.

Residual classification:

| Bucket | Representative evidence | Disposition |
| --- | --- | --- |
| Direct-global return authority predicate | `20041112-1 foo.block_1` still has raw `bir.ret ptr @global`, prepared `home @global value_id=4 kind=register reg=t0`, and a matching `BeforeReturn` `FunctionReturnAbi` move to `a0`. Step 3 focused tests cover this authority predicate and fail-closed edges. | Prepared authority is complete for the intended direct-global return shape. |
| Direct-global return consumer | Fresh RV64 object probe still fails with `unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering`. | In-scope remaining direct-global packet if narrowed to consuming explicit direct-global return authority only. |
| Direct-global select-chain facts | Prepared dump retains `direct_global_select_chain=yes` on call-argument/select-chain rows. | Candidate facts remain separate; do not bundle them into the direct-global return consumer unless explicitly selected. |
| Generic terminator/select publication | The object diagnostic is generic, but the representative can be classified by explicit direct-global return facts. | Out of scope for idea 440 unless constrained to direct-global authority consumption; generic terminator/select publication remains idea 441 work. |
| Store/global publication and layout facts | Immediate store/global layout facts are already visible in the dump. | Out of scope for idea 440; covered by earlier global publication plans. |

## Suggested Next

Keep idea 440 active only for the exact remaining direct-global return consumer
packet: materialize `bir.ret ptr @global` in the RV64 object route by consuming
`plan_prepared_direct_global_return_authority`/prepared homes and matching
`BeforeReturn` ABI move facts as authority. Fail closed for raw-only,
missing-authority, mismatched-home, non-global, unsupported-home, or non-GPR ABI
return shapes. If the supervisor does not want to extend this runbook with that
consumer packet, route it as a durable follow-up instead of closing the idea as
complete.

## Watchouts

- Do not implement generic RV64 terminator/select lowering in the direct-global
  return consumer packet.
- Do not infer return handling from raw `bir.ret ptr @global`, symbol spelling,
  testcase names, probe instruction indexes, or raw `BeforeReturn` moves.
- Keep `direct_global_select_chain=yes` rows separate from the first direct-global
  return consumer packet unless the supervisor explicitly selects a select-chain
  packet.
- Keep completed store/global layout/source publication work from earlier ideas
  out of this plan.
- Do not accept or modify `test_baseline.new.log`.
- Close-readiness decision: not close-ready as end-to-end object support; exact
  next packet exists for direct-global return authority consumption.

## Proof

Step 4 residual-disposition validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
