Status: Active
Source Idea Path: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current first bad fact for idea 361. The delegated
focused proof is now 7/7 green, including
`c_testsuite_aarch64_backend_src_00181_c`, so the old materialized
pointer-addressed `StoreLocal` writeback owner is not live in the current
tree.

## Suggested Next

Supervisor lifecycle routing can treat Step 1 as an already-green
classification and ask the plan owner whether idea 361 should close,
deactivate, or be replaced by a different source idea if broader current
evidence still needs ownership.

## Watchouts

- `build/backend_route/aarch64_pointer_value_named_scalar_writeback.s` still
  contains the focused materialized pointer writeback store
  `str w20, [x21]` through the computed address.
- `build/c_testsuite_aarch64_backend/src/00181.c.s` now emits computed stores
  in `Move`, including `str w9, [x10]` for destination writeback and source
  clearing, and the representative no longer times out under the delegated
  subset.
- No out-of-scope first bad fact was exposed by this packet.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log 2>&1
```

Result: build up to date; CTest passed 7/7. Proof log:
`test_after.log`.
