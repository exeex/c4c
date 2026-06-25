Status: Active
Source Idea Path: ideas/open/355_rv64_prepared_object_shape_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured RV64 Rejection Reasons

# Current Packet

## Just Finished

Step 2 added structured RV64 prepared-object rejection reasons without backend
capability repair:

- Added `RiscvPreparedObjectImageResult` and
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics` in
  `src/backend/mir/riscv/codegen/object_emission.hpp/.cpp`, preserving shared
  `prepared_consumer_category` plus diagnostic text through the prepared
  object-to-ELF writer handoff.
- Updated `src/backend/backend.cpp::emit_rv64_prepared_object_module` to keep
  the generic `RISC-V backend object route unsupported prepared module shape`
  prefix while appending `prepared_consumer_category=<stable_name>` for shared
  consumer-contract failures and appending RV64-local bucket text for target
  evidence.
- Replaced top-level empty RV64 object-result returns with stable local buckets
  for module string constants, unsupported function admission, unsupported
  parameter homes, unsupported stack frames/call frames, unsupported move-bundle
  target shapes, unsupported instruction fragments, unsupported terminator
  fragments, unsupported globals, and object module/ELF construction failure.
- Added focused coverage in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` that verifies the
  prepared ELF writer preserves an existing shared consumer diagnostic.
- Representative direct object command now records
  `module_string_constants: RV64 object route does not emit prepared string constants`
  in `test_after.log` for
  `tests/c/external/gcc_torture/src/20000112-1.c`.

## Suggested Next

Step 3 should add representative diagnostic tests for the named buckets without
expectation downgrades or testcase-name matching. Start with focused tests that
invoke backend/object diagnostics for `20000112-1.c` string constants and at
least one shared consumer-contract category, then add stable bucket assertions
for globals, non-i32 local memory/addressing, and declaration-entry handling as
the fixtures isolate those prepared shapes.

## Watchouts

- This packet intentionally did not repair globals, strings, ABI, widths,
  varargs, or lowering coverage.
- `unsupported_instruction_fragment` currently covers local memory
  width/addressing failures reached through instruction lowering; Step 3 tests
  should decide whether a narrower externally visible bucket is required by the
  source idea before widening the implementation.
- The legacy `write_rv64_prepared_relocatable_elf_object` API still returns
  only `std::optional` for existing callers; new diagnostics flow through
  `write_rv64_prepared_relocatable_elf_object_with_diagnostics`.

## Proof

Ran the delegated proof exactly:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_prepared_object_consumer_contract)$' && { build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000112-1.c -o /tmp/rv64-prepared-shape.o > test_after.log 2>&1; test $? -ne 0; }
```

Result: build passed, both focused CTest targets passed, and the representative
direct object command failed as expected with the specific
`module_string_constants` bucket in `test_after.log`.
