Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add focused indexed aggregate coverage and repair selected-address snapshot/writeback handoff

# Current Packet

## Just Finished

Step 2 repaired the AArch64 selected store-value publication handoff for
stack-homed selected values consumed by fixed `StoreGlobal` records. Global
store value publication now handles prepared frame-slot store-value operands,
emits the select-chain value into the stack home, tracks those publications to
avoid duplicate per-store emission, and prepublishes the stack homes for a
scalarized global select/store run before the first fixed global writeback
store consumes any of them.

Focused coverage was added to `backend_aarch64_instruction_dispatch`: a
scalarized global array-style fixture with two fixed global element stores now
requires both selected stack homes to be published before the first fixed
global store reloads `[sp, #16]`.

`00176` advanced past the localized uninitialized selected store-value
snapshot: generated `swap` assembly now emits the first batch of selected
`%t7...inner.store` publications to stack homes such as `[sp, #264]` through
`[sp, #324]` before the first fixed `array` store reloads `[sp, #264]`. The
next first bad fact is a runtime output mismatch after that publication repair;
the final observed sorted line was `1572863 65535 ... 65535`, indicating the
remaining failure is no longer the original read-before-publication of
uninitialized selected store-value homes.

## Suggested Next

Investigate the new `00176` first bad fact in the emitted select-chain
semantics for scalarized global array writeback. The selected homes are now
published before fixed stores, so the next packet should compare the emitted
nested select-chain control flow against the semantic BIR for the
`array[a] = array[b]` and `array[b] = tmp` batches rather than extending
store-value publication ordering.

## Watchouts

- Do not special-case `00176`, `swap`, one global symbol, one stack snapshot
  slot, one array index, one register, or one emitted instruction sequence.
- Preserve the new batch prepublication invariant: selected stack homes for a
  scalarized fixed-global store run must be filled before any fixed global
  store in that run reloads them.
- Preserve the formal-to-local publication repair from idea 353 and the prior
  indexed aggregate repairs for `00130`, `00187`, and `00195`.
- Reclassify if the first bad fact reaches call preservation, unsigned div/rem
  publication, formal/local publication, semantic admission, or runner policy.
- Do not assume prepared AArch64 currently has selected global element
  addresses for this store shape; the prepared evidence shows fixed
  `global_symbol` offsets plus selected store values.
- `c_testsuite_aarch64_backend_src_00187_c` still fails with the prior
  segmentation fault in this delegated proof and was not part of this repair.

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
`c_testsuite_aarch64_backend_src_00176_c` with the advanced runtime output
mismatch after selected store-value stack-home publication, and
`c_testsuite_aarch64_backend_src_00187_c` with the prior `RUNTIME_NONZERO`
segmentation fault. Canonical proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
