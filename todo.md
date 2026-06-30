Status: Active
Source Idea Path: ideas/open/439_store_source_global_memory_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Publication Packet

# Current Packet

## Just Finished

Completed Step 3: added focused producer/prepared coverage for global-memory
publication authority without editing RV64 target lowering.

Implemented:

- `prepare::prepared_global_symbol_memory_has_publication_authority`, requiring
  explicit global symbol identity, base-plus-offset support, size/alignment,
  global provenance identity, complete object extent, proven in-bounds range,
  and non-unknown/non-opaque layout authority.
- `prepare::prepared_store_global_publication_has_authority`, requiring a
  store-global publication, a destination access with global publication
  authority, and either explicit immediate-source encoding or a named
  store-source with current value home plus concrete producer metadata.

Focused tests in `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
cover:

- accepted coherent global memory authority;
- accepted binary-source global store publication with register value home;
- accepted explicitly encoded immediate source;
- fail-closed unknown layout authority, unknown-compatible range, non-global
  provenance, pointer-value memory, missing symbol identity, missing
  store-source producer, missing source home, local store-source publication,
  and implicit immediate source.

Remaining gap:

- The current store-global planner does not yet publish explicit
  immediate-source encoding for immediate global stores. The authority predicate
  accepts such a fact once present, but this packet did not implement that
  producer repair.

Artifact:

- `build/agent_state/439_step3_global_memory_publication_coverage/summary.md`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Recommended Step 4 packet: re-check the representative residuals against the
new authority predicates and decide whether this idea should close as
producer/prepared contract coverage or stay active for a narrow immediate
global-store source encoding producer packet. Do not edit RV64 target lowering
until representative prepared facts satisfy the authority predicates.

## Watchouts

- Do not infer store-source identity, global object identity, offset meaning,
  layout, addressability, or value home from raw BIR, testcase names, object
  labels, symbol spelling, or dump shape.
- Keep `source_producer=unknown` fail-closed until producer facts are explicit.
- Do not fold pointer-value memory, direct-global return/select-chain, or
  terminator/select publication work into this plan.
- Do not return to parked idea 442 for external-linkage formal provenance
  without a new source of closed-world/no-external-caller authority.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 3 code/test proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
