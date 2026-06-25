Status: Active
Source Idea Path: ideas/open/355_rv64_prepared_object_shape_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Representative Diagnostic Tests

# Current Packet

## Just Finished

Step 3 added representative diagnostic coverage for stable RV64 prepared-object
buckets without capability repair or testcase-name matching:

- Added focused assertions in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` for
  `module_string_constants`, `unsupported_global_data` for prepared global
  memory instructions and unsupported module global data, and
  `unsupported_local_memory_access` for non-32/64-bit local memory width.
- Reused the existing focused shared consumer-contract assertion for the
  preserved `UnsupportedParallelCopyExecutionSite` category through the
  prepared object-to-ELF writer.
- Added a diagnostic-only RV64 fallback in
  `src/backend/mir/riscv/codegen/object_emission.cpp` so failed prepared local
  load/store and global load/store lowering reports stable semantic buckets
  instead of the generic `unsupported_instruction_fragment` bucket.
- Representative direct commands now record structured diagnostics in
  `test_after.log`: `src/20000112-1.c` reports `module_string_constants`,
  `src/20000224-1.c` reports `unsupported_global_data`, and
  `src/20001203-1.c` reports `unsupported_local_memory_access`.

## Suggested Next

Step 4 should prove scan-bucket visibility with the supervisor-selected narrow
or scan command and decide whether declaration-entry and multi-block cases need
separate Step 4 evidence or a new packet, since this Step 3 packet only proved
the delegated string/global/local representatives plus the shared consumer
category.

## Watchouts

- This packet intentionally did not repair globals, strings, ABI, widths,
  varargs, declaration-entry control flow, or multi-block lowering coverage.
- The new local-memory diagnostic is only selected after normal instruction
  lowering fails and is based on prepared load/store width/address facts; it is
  not a lowering capability change.
- `src/20030216-1.c` and `src/20000113-1.c` were not part of the delegated
  direct-command proof. They are not recorded here as undiagnosable; they still
  need supervisor-routed evidence if Step 4 or a later packet requires them.

## Proof

Ran the delegated proof exactly:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_prepared_object_consumer_contract)$' && bash -lc ': > test_after.log; for case in src/20000112-1.c src/20000224-1.c src/20001203-1.c; do build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/$case -o /tmp/rv64-prepared-shape.o >> test_after.log 2>&1; status=$?; echo "CASE $case exit=$status" >> test_after.log; test $status -ne 0 || exit 1; done'
```

Result: build passed, both focused CTest targets passed, and all three
representative direct object commands failed as expected with structured
buckets in `test_after.log`.
