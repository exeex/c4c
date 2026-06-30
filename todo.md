Status: Active
Source Idea Path: ideas/open/447_immediate_global_store_source_encoding.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Immediate Source Packet

# Current Packet

## Just Finished

Completed Step 3: implemented the first immediate-source publication packet
for idea 447 and recorded supporting evidence under
`build/agent_state/447_step3_immediate_source_publication/`.

Implementation summary:

- `src/backend/prealloc/publication_plans.cpp` now publishes
  `PreparedEdgePublicationSourceProducerKind::Immediate` for immediate-valued
  store-global publication plans only after the destination global access
  already satisfies `prepared_global_symbol_memory_has_publication_authority`.
- The existing `prepared_store_global_publication_has_authority` predicate was
  preserved; it still rejects implicit immediates with `source_producer=unknown`.
- Focused coverage in
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp` proves a populated
  coherent immediate global store becomes explicitly authorized and an
  unknown-layout immediate global store remains fail-closed.

Representative proof:

- Fresh `20041112-1` prepared probe now prints
  `store_source function=main block=entry inst=0 source=<none> ... source_producer=immediate`.
- The matching destination row remains `symbol=global offset=0 size=4 align=4
  layout_authority=scalar_layout range_verdict=proven_in_bounds`.

## Suggested Next

Execute Step 4: residual disposition and close-readiness review for idea 447.
Re-probe representative immediate global-store rows, confirm
`20041112-1 main.entry.0` remains explicit `source_producer=immediate`, and
classify any remaining residuals as out of scope unless they are true
immediate-source encoding gaps.

## Watchouts

- Keep this plan limited to immediate global-store source encoding.
- Do not reopen scalar or integer-array global layout authority; those are
  closed by ideas 446 and 448.
- Do not infer immediate values or source publication in RV64 from raw BIR
  store shape, testcase names, block labels, symbol names, or one dump layout.
- Keep `source=<none>` and `source_producer=unknown` fail-closed until
  producer facts are explicit.
- Keep non-immediate global stores (`source=%t4`, `%t10`, `%t1`), local stores,
  pointer-value memory, direct-global return/select-chain rows, and target
  lowering outside Step 4.
- Preserve idea 439 store-publication predicates; do not weaken
  `prepared_store_global_publication_has_authority` to accept unknown sources.
- No RV64 target lowering changed in Step 3; Step 4 should not claim object
  route completion beyond the prepared immediate-source fact.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 3 implementation validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
