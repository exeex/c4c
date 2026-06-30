Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consume Direct-Global Return Authority

# Current Packet

## Just Finished

Completed Step 5: implemented the narrow RV64 direct-global return authority
consumer and recorded evidence under
`build/agent_state/440_step5_direct_global_return_consumer/`.

Implementation summary:

- `src/backend/mir/riscv/codegen/object_emission.cpp` now routes named pointer
  direct-global return candidates through
  `plan_prepared_direct_global_return_authority` before the RV64 object route
  accepts them.
- The consumer uses the prepared value home and matching `BeforeReturn`
  `FunctionReturnAbi` GPR move as authority. The move itself remains emitted by
  the existing before-return move-bundle traversal; the terminator emits the
  epilogue and `ret`.
- Ordinary non-global pointer returns continue through the existing generic GPR
  return path. Raw `@...` pointer returns without semantic global identity stay
  fail-closed instead of being accepted from spelling alone.
- Focused object tests cover accepted semantic direct-global return emission
  and fail-closed raw-only, missing-move, mismatched-move, unsupported-home, and
  unsupported-destination shapes.
- No direct-global select-chain consumer and no generic RV64 terminator/select
  lowering was implemented.

Representative probe:

- Fresh `20041112-1` artifacts under
  `build/agent_state/440_step5_direct_global_return_consumer/` still report
  `unsupported_terminator_fragment`. The focused tests prove the direct-global
  return consumer route; Step 6 should reclassify whether the representative is
  now blocked by direct-global select-chain work, another terminator/select
  residual, or out-of-scope idea 441 publication.

## Suggested Next

Execute Step 6: residual disposition and close-readiness review for idea 440.
Re-probe `20041112-1`, classify the remaining `unsupported_terminator_fragment`
after the direct-global return consumer landed, and decide whether any exact
direct-global select-chain packet remains in idea 440 or whether the rest belongs
to idea 441 terminator/select publication.

## Watchouts

- Do not treat the unchanged representative diagnostic as proof that
  direct-global return authority failed; the focused accepted object fixture
  proves that consumer route.
- Keep any next packet separated from generic RV64 terminator/select lowering.
- Reclassify `direct_global_select_chain=yes` rows before selecting a consumer
  packet; do not bundle them into return handling retroactively.
- Keep completed store/global layout/source publication work from earlier ideas
  out of this plan.
- Do not accept or modify `test_baseline.new.log`.
- Step 5 did not change `plan.md`, source ideas, expectations, allowlists, or
  `test_baseline.new.log`.

## Proof

Step 5 implementation validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Step 4 residual-disposition validation already passed before log roll-forward,
with backend regression guard reported as `327/327`.

Result: passed. Proof log: `test_after.log`.
