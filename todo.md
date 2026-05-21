Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Global Indexed Array Snapshot/Writeback Boundary

# Current Packet

## Just Finished

Step 1 localized `00176` `swap` to the global dynamic scalar-array
store-value publication/writeback boundary. Semantic BIR does not preserve a
selected global element address into AArch64 for `array[a] = array[b]`; the
dynamic global array store is scalarized into fixed
`load_global`/`select`/`store_global` operations for every element. Prepared BIR
therefore records fixed `base=global_symbol symbol=array offset=N` stores whose
stored values are `%t7.outer0.eltN.inner.store` select results. AArch64 memory
lowering then consumes those prepared store-value homes from frame slots such as
`slot#70+stack264` and `slot#71+stack268`, but the generated first store batch
reads `[sp, #264]` and `[sp, #268]` before any `cmp`/`csel` publication for
those `%t7...inner.store` values. The corruption is an uninitialized selected
store-value snapshot consumed by fixed global-symbol stores, not an incorrect
prologue frame slot.

Evidence:

- `/tmp/00176.swap.bir.txt`: `%t7.outer0.elt0.inner.store = bir.select eq i64
  %t6, 0, i32 %t5.outer0, %t7.outer0.elt0`, followed by `bir.store_global
  @array, i32 %t7.outer0.elt0.inner.store`; element offsets 4..60 repeat the
  same select-then-fixed-store pattern.
- `/tmp/00176.swap.prepared.txt`: `access block=entry inst_index=68
  base=global_symbol stored=%t7.outer0.elt0.inner.store symbol=array offset=0`;
  `%t7.outer0.elt0.inner.store` has home `slot#70+stack264`, and
  `%t7.outer0.elt1.inner.store` has home `slot#71+stack268`.
- `/tmp/00176.s`: the first `array[a] = array[b]` store batch emits
  `ldr w20, [array]`, then `ldr w10, [sp, #264]` / `str w10, [array]`, then
  `ldr w10, [sp, #268]` / `str w10, [array+4]`, with no preceding select
  publication. The later `array[b] = tmp` batch does emit `cmp`/`csel` and
  publishes `%t10...inner.store` values to `[sp, #328+]` before storing, so the
  failure is specific to the first dynamic-global store-value publication path.

Likely Step 2/3 implementation surface:

- BIR scalarization/address ownership:
  `src/backend/bir/lir_to_bir/memory/value_materialization.cpp`
  (`append_dynamic_global_scalar_array_store`,
  `load_dynamic_global_scalar_array_value`, `synthesize_value_array_selects`);
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` dynamic-global
  load/store delegation; `src/backend/bir/lir_to_bir/memory/addressing.cpp`
  dynamic global aggregate/scalar array access discovery.
- AArch64 publication/memory handoff:
  `src/backend/mir/aarch64/codegen/alu.cpp`
  (`lower_scalar_select_publication`);
  `src/backend/mir/aarch64/codegen/dispatch.cpp` scalar-control lowering order;
  `src/backend/mir/aarch64/codegen/memory.cpp`
  (`make_prepared_store_memory_instruction_record`,
  `make_memory_record_from_prepared_access`, `lower_memory_instruction`).

## Suggested Next

Use the first implementation packet to make AArch64 publish dynamic global
store select values before fixed global-symbol stores consume their frame-slot
homes, or replace the BIR scalarized writeback with a real selected-address
writeback if Step 3 chooses to repair the earlier BIR boundary. The narrow
proof subset for that packet should keep the delegated focus:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00130_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00195_c)$' | tee test_after.log`

## Watchouts

- Do not special-case `00176`, `swap`, one global symbol, one stack snapshot
  slot, one array index, one register, or one emitted instruction sequence.
- Preserve the formal-to-local publication repair from idea 353 and the prior
  indexed aggregate repairs for `00130`, `00187`, and `00195`.
- Reclassify if the first bad fact reaches call preservation, unsigned div/rem
  publication, formal/local publication, semantic admission, or runner policy.
- Do not assume prepared AArch64 currently has selected global element
  addresses for this store shape; the prepared evidence shows fixed
  `global_symbol` offsets plus selected store values.
- If repairing only AArch64 publication order, ensure true/false operands for
  `%t7...inner.store` are materialized as well; `%t5.outer0` is itself a
  dynamic-load select chain.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00130_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00195_c)$' | tee test_after.log`

Result: build was up to date; CTest ran 8 tests, 6 passed and 2 failed.
Passing: `backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00130_c`,
`c_testsuite_aarch64_backend_src_00195_c`. Failing:
`c_testsuite_aarch64_backend_src_00176_c` with the localized runtime output
mismatch, and `c_testsuite_aarch64_backend_src_00187_c` with
`RUNTIME_NONZERO` segmentation fault. Canonical proof log:
`test_after.log`.
