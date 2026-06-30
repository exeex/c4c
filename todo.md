Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 6: re-probed `20041112-1` after the reviewed Step 5
direct-global return consumer and recorded final residual disposition under
`build/agent_state/440_step6_final_residual_disposition/`.

Residual classification:

| Bucket | Representative evidence | Disposition |
| --- | --- | --- |
| Direct-global return authority | `foo.block_1` still has `bir.ret ptr @global`, prepared `home @global value_id=4 kind=register reg=t0`, and the matching `BeforeReturn` GPR ABI move to `a0`. Step 5 focused object tests accept this authority shape. | Complete for idea 440. |
| Direct-global return fail-closed cases | Step 5 tests reject raw-only spelling, missing/mismatched return moves, unsupported homes, and unsupported return destinations; Step 3 prepared tests cover predicate-only edges. | Covered fail-closed. |
| Direct-global select-chain facts | Fresh prepared dump still has `direct_global_select_chain=yes` on call-argument/select-chain/store-source rows. | Candidate facts remain visible, but the current object diagnostic does not isolate an exact direct-global select-chain consumer gap. |
| Remaining object-route failure | Fresh object probe still reports `unsupported_terminator_fragment`; fresh prepared control flow shows `bar.entry` has `%t6 = bir.ne ptr %t2, %t5` feeding `bir.cond_br i32 %t6, block_4, block_5`. | Route to idea 441 terminator/select publication authority, not more direct-global return work. |
| Store/global publication and layout | Prior global layout/source facts remain visible. | Out of scope for idea 440. |

## Suggested Next

Recommend plan-owner close idea 440 as complete for direct-global return
authority. Route the remaining `20041112-1` object-route failure to
`ideas/open/441_terminator_select_publication_authority.md`. Keep idea 440
active only if a future probe isolates an exact direct-global select-chain
consumer gap independent of generic terminator/select publication.

## Watchouts

- Do not reopen direct-global return authority based on the unchanged
  representative diagnostic; fresh classification points at the pointer-compare
  conditional branch in `bar.entry`.
- Do not select generic RV64 terminator/select lowering under idea 440.
- If plan-owner continues with idea 441, preserve the same no-overfit rule:
  consume explicit publication facts rather than raw branch/select shape.
- Do not accept or modify `test_baseline.new.log`.
- Step 6 did not change `plan.md`, source ideas, implementation files, tests,
  expectations, allowlists, `review/`, `test_baseline.new.log`, or
  `test_before.log`.

## Proof

Step 6 final residual-disposition validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
