Status: Active
Source Idea Path: ideas/open/448_array_aggregate_global_layout_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Non-Scalar Authority Packet

# Current Packet

## Just Finished

Completed Step 3: implemented the selected `Publish Integer-Array Global
Layout Authority` packet.

Implementation summary:

| area | result |
| --- | --- |
| Producer carrier | Added durable integer-array facts to `bir::Global`: authority bit, element size, and element count. |
| Producer population | Populated integer-array authority only from the integer-array lowering branch in `src/backend/bir/lir_to_bir/globals.cpp`. |
| Prepared publication | `src/backend/prealloc/stack_layout/coordinator.cpp` now publishes `ByteStorageAggregate` only for semantic, non-extern, non-TLS integer-array globals with coherent complete extent, base-plus-offset access, element-width access, element-aligned offset, and `ProvenInBounds` range. |
| Accepted coverage | Focused BIR/prepared coverage proves an in-bounds single-element integer-array global load becomes target-consumable under `prepared_global_symbol_memory_has_publication_authority`. |
| Fail-closed coverage | Scalar globals remain separate, aggregate lanes remain unknown, and out-of-range, misaligned offset, partial-width, extern, TLS, missing-extent, raw-spelling, pointer-value/direct-global, and immediate-source boundaries remain rejected. |

Representative probe:

- `build/agent_state/448_step3_integer_array_global_layout_authority/930930-1.prepared.out`

The `930930-1 @mem+792` prepared row now carries
`layout_authority=byte_storage_aggregate` with `range_verdict=proven_in_bounds`.

Artifacts:

- `build/agent_state/448_step3_integer_array_global_layout_authority/summary.md`
- `build/agent_state/448_step3_integer_array_global_layout_authority/930930-1.prepared.out`
- `build/agent_state/448_step3_integer_array_global_layout_authority/930930-1.prepared.err`
- `test_after.log`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Re-probe or inspect:

- `930930-1 @mem+792`: should remain accepted for integer-array global layout
  authority, while later object-route failures should be classified by their
  first remaining owner.
- Scalar `20041112-1 @global+0`: should remain out of scope, already closed by
  idea 446.
- `20000622-1`: should remain out of scope for global layout authority.

Decide whether idea 448 is close-ready for integer-array global layout
authority, should remain active for structured aggregate authority, or should
split structured aggregate support into a separate idea.

## Watchouts

- Do not edit RV64 target lowering to infer non-scalar layout authority.
- Do not broaden this implementation into structured aggregate layout; split if
  aggregate support is needed.
- Do not treat `ByteStorageAggregate` integer-array authority as permission for
  arbitrary aggregate byte lanes, pointer arrays, strings, extern/TLS globals,
  or raw-only unresolved symbols.
- Keep immediate global-store source encoding with
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Keep direct-global return/select-chain, pointer-value memory,
  terminator/select publication, and local/frame-slot residuals outside this
  idea.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 3 implementation proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset output.
