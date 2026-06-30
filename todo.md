Status: Active
Source Idea Path: ideas/open/446_global_memory_layout_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed representative rows and classified residuals after
scalar global layout authority publication.

Representative disposition:

| row | Step 4 evidence | disposition |
| --- | --- | --- |
| `20041112-1 @global+0` | Fresh prepared dump shows `layout_authority=scalar_layout` with `range_verdict=proven_in_bounds` for scalar global load/store rows in `foo` and `bar`, plus the `main.entry` address row. Object route now fails at `unsupported_terminator_fragment`. | Accepted for scalar global layout authority; remaining object-route residual is outside idea 446. |
| `930930-1 @mem+792` | Fresh prepared dump still shows `layout_authority=unknown`, width 8, align 8, `range_verdict=proven_in_bounds`. | Correctly fail-closed for this scalar packet. Needs a separate array/aggregate global layout-authority producer packet or split if plan-owner wants to pursue it. |
| `20000622-1` | Fresh prepared dump has frame-slot rows only; no global-symbol memory row. Object route still fails at unrelated unsupported instruction lowering. | Out of scope for this global layout-authority idea. |

Artifacts:

- `build/agent_state/446_step4_residual_disposition/classification.md`
- `build/agent_state/446_step4_residual_disposition/probe_summary.tsv`
- `build/agent_state/446_step4_residual_disposition/20041112-1.prepared.out`
- `build/agent_state/446_step4_residual_disposition/20041112-1.object.err`
- `build/agent_state/446_step4_residual_disposition/930930-1.prepared.out`
- `build/agent_state/446_step4_residual_disposition/930930-1.object.err`
- `build/agent_state/446_step4_residual_disposition/20000622-1.prepared.out`
- `build/agent_state/446_step4_residual_disposition/20000622-1.object.err`
- `test_after.log`

Decision: idea 446 appears close-ready for scalar global layout authority. The
remaining `930930-1 @mem+792` evidence is not a defect in the scalar packet; it
is a non-scalar array/aggregate global layout-authority question and should be
split or separately activated rather than widened into this close-readiness
slice.

## Suggested Next

Plan-owner close-readiness review.

Recommended lifecycle disposition: close idea 446 as complete for scalar
global layout authority, with an optional follow-up source idea for
array/aggregate global layout authority if `930930-1 @mem+792` should become
target-consumable later.

## Watchouts

- Do not edit RV64 target lowering to infer global layout authority.
- Do not treat `930930-1 @mem+792` as covered by scalar global authority.
- Keep immediate global-store source encoding with
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Keep direct-global return/select-chain, terminator publication,
  pointer-value memory, and local/frame-slot residuals outside this idea.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 4 residual-disposition proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset output.
