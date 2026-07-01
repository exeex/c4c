Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish Semantic Admission From Global/Static GEP Authority

# Current Packet

## Just Finished

Step 4 published final semantic global/static GEP admission records from the
Step 3 `global_static_gep_authority` certificate surface. The new
`GlobalStaticSemanticGepRecord` is a consumer projection of
`GlobalStaticGepAuthorityRecord`, defaults fail-closed, preserves non-available
authority statuses, and becomes `Available` only when the referenced authority
record is available and still carries complete global identity, layout path,
element byte range, range verdict, and LIR producer coordinate facts.

`populate_global_static_semantic_geps` now publishes records from each
function's authority records and rejects duplicate authority identities as
`PreparedBirCoordinateConfusion`. The existing scalar local-load publication
flow invokes the new global/static semantic GEP population pass, so normal
prealloc publication reaches this surface without RV64/MIR or object-emission
consumer work.

Evidence summary:
`build/agent_state/500_step4_semantic_global_static_gep_admission/summary.md`.

## Suggested Next

Ask the plan owner to decide residual lifecycle disposition for idea 500 after
Step 4. The expected Step 5 packet is residual disposition: close idea 500 if
the final semantic global/static GEP admission producer is complete, or route a
new focused consumer/lowering idea if downstream RV64/MIR/object work should
resume from `global_static_semantic_geps`.

## Watchouts

- This packet intentionally does not implement RV64/MIR lowering or object
  emission consumers.
- Final admission is still certificate-backed only; do not reconstruct it from
  prepared object data, final homes, relocations, raw testcase shape, object
  files, or target behavior.
- Pointer/string, runtime/string intrinsic, raw-shape-only, target-only, and
  coordinate-confusion statuses remain fail-closed unless a later lifecycle
  packet adds a matching lower authority or consumer-specific prerequisite.

## Proof

Step 4 implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
