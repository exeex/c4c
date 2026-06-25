Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Target Data Section and Symbol Emission

# Current Packet

## Just Finished

Completed Plan Step 2: added RV64 object-model data section and symbol emission
for prepared string constants plus supported prepared global storage.

The object route now emits:

- prepared string constants into `.rodata`, using structured `TextId` spelling
  when available, appending a trailing NUL only when the prepared bytes do not
  already include one, and defining local object symbols with size and offset
  in the read-only section.
- immediate scalar globals into `.rodata` for constant globals and `.data` for
  mutable globals, preserving prepared link-name spelling, binding, object kind,
  size, offset, and alignment.
- immediate linear initializer-element globals into `.rodata`, `.data`, or
  `.bss`; mutable all-zero element storage is reserved in `.bss`.
- extern no-storage globals without forcing section emission, so string-pool
  companion globals no longer cause a false unsupported-shape rejection.

Unsupported shapes still fail closed with `unsupported_global_data`, including
TLS/GOT-required globals, pointer/name initializers that need data relocations,
duplicate/unnamed data symbols, zero alignment, and non-immediate or size-mismatched
initializer streams.

## Suggested Next

Delegate Step 3: add RV64 code/data relocations for prepared string/global
address uses. Keep the packet limited to relocation records and address
materialization support; do not broaden into global load/store instruction
lowering unless the supervisor explicitly includes that work.

## Watchouts

- `src/20000112-1.c` and `src/20000223-1.c` now move past the old
  `module_string_constants` rejection and fail later in text lowering
  (`unsupported_terminator_fragment` and `unsupported_instruction_fragment`).
- `src/20000224-1.c` and `src/20000227-1.c` still fail at global load/store
  instruction admission, as expected for this Step 2 boundary.
- Pointer/name initializers are intentionally still unsupported until data
  relocations are owned.
- The focused tests cover object sections, symbol kind/binding/size/offset,
  section alignment, and ELF writer success for strings, writable scalar data,
  linear byte globals, BSS reservation, and existing constant F64 storage.

## Proof

Proof ran and passed:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && bash -lc ': > test_after.log; for case in src/20000112-1.c src/20000223-1.c src/20000224-1.c src/20000227-1.c; do build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/$case -o /tmp/rv64-data-step2.o >> test_after.log 2>&1; status=$?; echo "CASE $case exit=$status" >> test_after.log; done'
```

`test_after.log` is preserved. It records the two string representative cases
moving past `module_string_constants` to later unsupported text-lowering
buckets, and the two global memory instruction cases remaining at
`unsupported_global_data` until a later packet owns instruction lowering.
