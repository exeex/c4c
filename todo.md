Status: Active
Source Idea Path: ideas/open/446_global_memory_layout_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Layout Authority Packet

# Current Packet

## Just Finished

Completed Step 3: implemented the selected `Publish Scalar Global Layout
Authority` producer/prepared packet.

Implementation summary:

| area | result |
| --- | --- |
| Producer carrier | Added `bir::Global::has_scalar_layout_authority` and populated it only from scalar LIR global lowering. |
| Prepared publication | `src/backend/prealloc/stack_layout/coordinator.cpp` now publishes `ScalarLayout` for prepared global-symbol memory only when the resolved semantic global has scalar authority, is not extern/TLS, has nonzero size/alignment, the prepared address is base-plus-offset-capable, object extent is complete, and the requested range is already `ProvenInBounds`. |
| Accepted coverage | Focused BIR/prepared coverage proves in-bounds scalar global load and store accesses become target-consumable under `prepared_global_symbol_memory_has_publication_authority`. |
| Fail-closed coverage | Aggregate global lanes, out-of-range scalar accesses, missing-extent scalar-shaped globals, raw spelling drift, pointer-value memory, direct-global materialization rows, and implicit immediate-source store publication remain rejected. |

Artifacts:

- `build/agent_state/446_step3_scalar_global_layout_authority/summary.md`
- `test_after.log`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Re-probe or inspect the representative rows from the Step 1/439 evidence:

- `20041112-1 @global+0`: should now be the primary scalar-global acceptance
  candidate if the prepared route carries the new scalar authority from LIR
  lowering.
- `930930-1 @mem+792`: should remain fail-closed unless producer facts prove a
  later array/aggregate global layout-authority packet is justified.
- `20000622-1`: should remain out of scope for this global layout-authority
  idea.

Decide whether this idea is close-ready for scalar global layout authority, must
keep active for an array/aggregate global authority packet, or should split that
remaining work into a separate source idea.

## Watchouts

- Do not edit RV64 target lowering for residual rows.
- Do not infer global layout, extent, addressability, offsets, or symbol
  identity from representative filenames, raw BIR, object labels, or dump
  positions.
- Keep arrays, aggregates, strings, extern declarations, TLS globals,
  unresolved/raw-only symbols, missing extents, out-of-range accesses, and
  `OpaqueCompatibility` fail-closed unless a later producer packet supplies
  explicit authority.
- Immediate global-store source encoding remains
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 3 implementation proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset output.
