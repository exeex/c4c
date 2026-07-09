Status: Active
Source Idea Path: ideas/open/621_rv64_prepared_global_value_location_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Breadth And Guards

# Current Packet

## Just Finished

Step 4 narrowly classified why `src/pr91137.c` still stops at the prepared
global facts gate after the Step 3 consumer implementation.

Fresh diagnostics:

- `--dump-bir` and `--dump-prepared-bir` both succeed for
  `tests/c/external/gcc_torture/src/pr91137.c`.
- `--codegen obj` still fails with
  `unsupported_global_data: RV64 object route requires supported prepared
  global memory facts`.
- The first relevant unsupported prepared-global fact is in `fn2`,
  `block_6`, `inst_index=4`: `bir.store_global @c, i32
  %t22.outer0.elt0.inner.store`.
- The memory access fact for that store is complete/direct:
  `base=global_symbol`, `stored=%t22.outer0.elt0.inner.store`, `symbol=c`,
  `offset=0`, `size=4`, `align=4`, `base_plus_offset=yes`,
  `layout_authority=byte_storage_aggregate`,
  `range_verdict=proven_in_bounds`.
- The unsupported part is the store source publication, not the global memory
  access authority: `store_source function=fn2 block=block_6 inst=4` is
  `status=available`, `intent=store_global_publication`,
  `source_producer=select_materialization`, `source_freshness_status=selected`,
  `direct_global_select_chain=yes`, `direct_global_root_is_select=yes`,
  `direct_global_root_inst=3`.
- The stored value has a prepared GPR home (`%t22.outer0.elt0.inner.store`
  in `s2`), and the source load feeding the select also has complete direct
  global facts (`block_6 inst_index=2`, `symbol=c`, `offset=0`, `size=4`).

Classification: `src/pr91137.c` is no longer blocked by missing prepared/global
producer access facts for the first store. It belongs to selected/direct-global
store-publication policy or downstream source-materialization ownership, not a
producer/access-authority follow-up and not an unambiguous continuation of the
active prepared-global value-location consumer route.

## Suggested Next

Supervisor should route `src/pr91137.c` out of this consumer slice unless a
reviewer finds that selected/direct-global `store_global_publication` source
materialization is explicitly in scope for idea 621.

## Watchouts

- Do not treat `src/pr91137.c` as a producer/access-authority gap based only on
  the generic object-route diagnostic; the first inspected access is complete.
- Do not widen Step 3 into selected/direct-global store-publication source
  materialization without supervisor/reviewer confirmation.
- Keep `src/ieee/20001122-1.c` guard-only unless a separate width-policy owner
  resolves the unsupported access size.
- Keep `src/991030-1.c` out of this route because it already emits an object
  successfully.

## Proof

Focused probes run:

`./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-bir tests/c/external/gcc_torture/src/pr91137.c`

`./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/pr91137.c`

`./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr91137.c -o /tmp/c4c_pr91137_classify/pr91137.o`

Focused diagnostic logs: `/tmp/c4c_pr91137_classify/pr91137.bir.txt`,
`/tmp/c4c_pr91137_classify/pr91137.prepared.txt`, and
`/tmp/c4c_pr91137_classify/pr91137.obj.err`.

Delegated proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. The build was up to date; CTest reported
`100% tests passed, 0 tests failed out of 347`. Proof log: `test_after.log`.
