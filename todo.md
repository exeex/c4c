Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Reclassify Residuals

# Current Packet

## Just Finished

Step 4 localized the remaining `00176` runtime mismatch without implementation
changes. The old fixed-store-value publication failure has advanced: generated
`swap` still emits the `%t7...inner.store` publication batch to stack homes such
as `[sp, #264]` before the first fixed `array` store reloads those homes.

The new `00176` first bad fact is earlier in `swap`, at the load-side selected
value used for `tmp = array[a]`. Semantic and prepared BIR both contain the
load select chain:

`%t2.outer0 = bir.select eq i64 %t0, 0, i32 %t2.outer0.elt0, %t2.outer0.sel1`

followed by `bir.store_local %lv.tmp, i32 %t2.outer0`. Prepared storage assigns
`%t2.outer0` to stack offset `124`, and `%lv.tmp` is the local home at `[sp]`.
Generated assembly loads every fixed global element into `[sp, #4]` through
`[sp, #64]`, then immediately does:

`ldr w9, [sp, #124]`
`str w9, [sp]`

No instruction writes `[sp, #124]` before that load. The generated code does
materialize later selected store-value homes such as `[sp, #264]`, but the
initial load-side selected snapshot for `%t2.outer0` is consumed uninitialized.
This keeps `00176` in scope for idea 348 as selected indexed aggregate
value/snapshot materialization, but the owner is now the load-side selected
result publication into the local snapshot consumed by `store_local`, not the
fixed global store-value prepublication repaired in Step 2.

`00187` was checked and does not reach this owner. Its prepared BIR/metadata
center on local buffer plus external calls, and generated `main` reloads string
symbol argument homes from `[sp, #32]`, `[sp, #40]`, `[sp, #48]`, and similar
slots before those homes are populated, then passes them to `fopen`/`printf`.
The runtime result remains `RUNTIME_NONZERO` segmentation fault, so `00187`
stays separately classified as a call/symbol publication or preservation
boundary rather than indexed aggregate selected-address/writeback.

## Suggested Next

Repair the AArch64 publication/materialization path for load-side selected
scalarized global aggregate values that are stack-homed and immediately
consumed by `store_local` snapshots. Use a focused backend case that proves a
selected global array load result is written to its prepared home before a local
snapshot reload consumes it, then re-run `00176` and the existing Step 4 subset.

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
- Do not fold `00187` into the next indexed aggregate packet. Its current
  evidence reaches external call argument/symbol home publication, not the
  `00176` selected global array load snapshot.

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
segmentation fault. The fresh `00176` final line was
`65535 3670015 74 74 74 74 74 74 74 74 74 74 74 74 74 74`, consistent with an
uninitialized load-side selected snapshot. Canonical proof log:
`test_after.log`.
