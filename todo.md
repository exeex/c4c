Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consume Direct-Global Return Authority

# Current Packet

## Just Finished

Plan-owner review accepted the Step 4 residual disposition and kept idea 440
active for the exact remaining direct-global return consumer packet.

Step 4 re-probed representative direct-global return/select-chain evidence and
recorded residual disposition under
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

Execute Step 5: implement the exact direct-global return consumer packet.
Materialize `bir.ret ptr @global` in the RV64 object route only by consuming
`plan_prepared_direct_global_return_authority`, the prepared global
home/storage, and the matching `BeforeReturn` `FunctionReturnAbi` GPR move
facts as authority.

Required fail-closed coverage:

- raw-only direct-global return with no prepared authority;
- missing prepared authority;
- mismatched value home or value id;
- non-global pointer return;
- unsupported home/storage;
- non-pointer return;
- non-GPR or otherwise unsupported ABI return shape.

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

Step 5 expected proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Step 4 residual-disposition validation already passed before log roll-forward,
with backend regression guard reported as `327/327`.
