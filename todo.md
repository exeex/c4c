Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Global-Symbol Memory-Access Facts

# Current Packet

## Just Finished

Step 3 stamped `bir::MemoryAddress::base_link_name_id` from
`global_it->second.link_name_id` in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` for aggregate
`LirOperandKind::Global` load lanes emitted as `LoadLocalInst` facts.

The producer now preserves semantic global-symbol identity alongside the
existing raw symbol spelling, byte offset, size, alignment, and default address
facts. No RV64 fallback inference from raw `LoadLocalInst addr <global>`
spelling was added, and the raw/no-id prepared-contract test remains
fail-closed.

## Suggested Next

Run the representative `src/20030914-2.c` RV64 torture proof to confirm the
prepared global-symbol publication reaches the previous boundary, then classify
the next residual shape or close/escalate the runbook if no in-scope boundary
remains.

## Watchouts

- The new prepared-contract test intentionally leaves the raw/no-id
  `LoadLocalInst` global lane unpublished, so future work should not weaken
  that fail-closed behavior.
- RV64 object emission should continue consuming explicit prepared facts and
  must not infer global-symbol memory facts from raw spelling.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

Delegated proof ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'; } > test_after.log 2>&1
```

Result: passed. Test subset: `backend_prepare_stack_layout` and
`backend_riscv_object_emission`. Proof log: `test_after.log`.
