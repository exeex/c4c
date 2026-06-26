Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Prepared-Contract Coverage

# Current Packet

## Just Finished

Step 2 added focused prepared-contract coverage in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp` for ordinary
`LoadLocalInst` global-address lanes. The structured lane uses
`bir::MemoryAddress::BaseKind::GlobalSymbol` with a valid
`base_link_name_id` and now proves publication of
`PreparedAddressBaseKind::GlobalSymbol`, prepared symbol identity, byte offset,
size, alignment, default address-space/volatile facts, base-plus-offset
support, direct global materialization policy, provenance base identity,
complete object extent, requested byte range, and an in-bounds range verdict.

The same fixture also preserves fail-closed coverage for a matching structured
global lane that carries only raw spelling and no valid `base_link_name_id`.
The coverage is semantic and uses a generic aggregate symbol and lane offset,
not the representative torture testcase spelling.

## Suggested Next

Delegate Step 3 to stamp `MemoryAddress::base_link_name_id` from the already
available `global_it->second.link_name_id` in the aggregate-load producer for
`LirOperandKind::Global`, then prove the producer emits the structured lane
facts that this prepared-contract test now covers.

## Watchouts

- Do not add target-side fallback inference from raw `LoadLocalInst addr
  <global>` spelling; idea 383 closed that route as fail-closed.
- The producer fix should use `global_it->second.link_name_id`, not a raw-name
  lookup in prepared stack layout. The existing raw-spelling compatibility
  path is intentionally narrow and should not become the normal publication
  route for semantic globals.
- The new prepared-contract test intentionally leaves the raw/no-id
  `LoadLocalInst` global lane unpublished, so a future producer slice should
  not weaken that fail-closed behavior.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

Delegated proof ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'; } > test_after.log 2>&1
```

Result: passed. Test subset: `backend_prepare_stack_layout`. Proof log:
`test_after.log`.
