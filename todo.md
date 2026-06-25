Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add RV64 Code/Data Relocations

# Current Packet

## Just Finished

Completed Plan Step 3: added RV64 object-model relocation support for prepared
string/global address materialization without admitting broad global load/store
instruction lowering.

The object route now emits PC-relative `AUIPC`/`ADDI` relocation pairs for
prepared `StringConstant` and direct-global address materializations, selects
the prepared destination GPR, records object-vs-function fixup target kind, and
lets relocation-created undefined object placeholders be completed by later
`.rodata`/`.data` symbol definition. Focused RV64 object-emission tests prove
prepared string and global address materializations target defined object
symbols and serialize through the ELF writer.

Unsupported pointer/name initializers still fail closed in the data emitter,
and `LoadGlobalInst`/`StoreGlobalInst` lowering remains outside this packet.

## Suggested Next

Delegate Step 4: add the next narrow RV64 allowlist packet for global data
uses, starting with the smallest prepared global memory instruction shape the
supervisor chooses. Keep it separate from pointer/name data initializers and do
not weaken unsupported-case expectations.

## Watchouts

- `src/20000112-1.c` and `src/20000223-1.c` still move past string/data
  admission and fail later in text lowering (`unsupported_terminator_fragment`
  and `unsupported_instruction_fragment`).
- `src/20000224-1.c` and `src/20000227-1.c` still fail at global load/store
  instruction admission, which is the expected boundary for Step 3.
- Pointer/name initializers remain intentionally unsupported; this packet only
  covers code relocations for prepared address materializations.

## Proof

Proof ran and passed:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && bash -lc ': > test_after.log; for case in src/20000112-1.c src/20000223-1.c src/20000224-1.c src/20000227-1.c; do build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/$case -o /tmp/rv64-data-step3.o >> test_after.log 2>&1; status=$?; echo "CASE $case exit=$status" >> test_after.log; done'
```

`test_after.log` is preserved. It records the two string representative cases
at later unsupported text-lowering buckets and the two global memory
instruction representatives remaining at `unsupported_global_data` until a
later packet owns instruction lowering.
