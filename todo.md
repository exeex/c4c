Status: Active
Source Idea Path: ideas/open/448_array_aggregate_global_layout_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed representative rows and classified residuals after
integer-array global layout authority publication.

Representative disposition:

| row | Step 4 evidence | disposition |
| --- | --- | --- |
| `930930-1 @mem+792` | Fresh prepared dump shows `layout_authority=byte_storage_aggregate` with `range_verdict=proven_in_bounds` for the `main.entry.0` global store. Object route still fails at `unsupported_instruction_fragment`; prepared evidence shows frame-slot call args, pointer-value memory, local store publication, and unsupported destination-storage residuals. | Accepted for integer-array global layout authority; remaining failures are outside idea 448. |
| `20041112-1 @global+0` | Fresh prepared dump remains `layout_authority=scalar_layout` for scalar rows; object route fails at `unsupported_terminator_fragment`. | Out of scope; scalar layout authority was closed by idea 446, and remaining direct-global/terminator/immediate-source owners are separate. |
| `20000622-1` | Fresh prepared dump has frame-slot rows only and no global-symbol memory row; object route fails at `unsupported_instruction_fragment`. | Out of scope for global layout authority. |

Artifacts:

- `build/agent_state/448_step4_residual_disposition/classification.md`
- `build/agent_state/448_step4_residual_disposition/probe_summary.tsv`
- `build/agent_state/448_step4_residual_disposition/930930-1.prepared.out`
- `build/agent_state/448_step4_residual_disposition/930930-1.object.err`
- `build/agent_state/448_step4_residual_disposition/20041112-1.prepared.out`
- `build/agent_state/448_step4_residual_disposition/20041112-1.object.err`
- `build/agent_state/448_step4_residual_disposition/20000622-1.prepared.out`
- `build/agent_state/448_step4_residual_disposition/20000622-1.object.err`
- `test_after.log`

Decision: idea 448 appears close-ready for integer-array global layout
authority. The plan should not remain active for RV64 target-consumer work or
structured aggregate layout. Structured aggregate authority should be split into
a separate source idea if needed.

## Suggested Next

Plan-owner close-readiness review.

Recommended lifecycle disposition: close idea 448 as complete for
integer-array global layout authority. Split future structured aggregate layout
authority if a new representative or source idea justifies it.

## Watchouts

- Do not edit RV64 target lowering to infer or consume layout authority in this
  close-readiness packet.
- Do not treat `ByteStorageAggregate` integer-array authority as permission for
  arbitrary structured aggregate byte lanes, pointer arrays, strings,
  extern/TLS globals, or raw-only unresolved symbols.
- Keep immediate global-store source encoding with
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Keep direct-global return/select-chain, pointer-value memory,
  terminator/select publication, local/frame-slot residuals, and frame-slot
  call-argument publication outside this idea.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 4 residual-disposition proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset output.
