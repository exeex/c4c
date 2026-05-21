Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 2-3-follow-up
Current Step Title: Repair load-side selected snapshot publication

# Current Packet

## Just Finished

Step 2/3 follow-up repaired the AArch64 load-side selected snapshot publication
path for stack-homed selected global aggregate load results consumed by
`store_local`. `lower_store_local_value_publication` now handles a stack-home
source operand when the stored value is a select chain containing direct global
loads: it materializes the selected value, stores it to the prepared source
stack home, and only then lets the local snapshot store reload that home.

The select-chain value emitter now prefers an already prepared stack home for
stack-homed `load_global` results, so publication reads the earlier fixed
global-load snapshot instead of re-reading the global symbol. Focused dispatch
coverage proves a nested selected global-load result is written to `[sp, #16]`
before the local store reloads `[sp, #16]` and stores to `[sp, #40]`.

The delegated proof advances `00176` past the prior `%t2.outer0` uninitialized
`[sp, #124]` mismatch: `c_testsuite_aarch64_backend_src_00176_c` now passes.
The only remaining failure in the delegated subset is the prior
`c_testsuite_aarch64_backend_src_00187_c` `RUNTIME_NONZERO` segmentation fault,
which remains classified outside this load-side selected snapshot owner.

## Suggested Next

Ask the supervisor/plan owner to decide whether idea 348 is now complete for
the indexed aggregate selected-snapshot route or whether a separate packet
should address the remaining `00187` call/symbol publication failure outside
this owner.

## Watchouts

- Do not fold the remaining `00187` segmentation fault back into indexed
  aggregate selected-address/writeback without new evidence; the latest
  classification points at external call argument/symbol home publication.
- Preserve the fixed-global store-value prepublication invariant: selected
  stack homes for a scalarized fixed-global store run must be filled before any
  fixed global store in that run reloads them.
- The new load-side path is intentionally semantic: it detects selected chains
  containing direct global loads and stack-home store-local source operands. Do
  not narrow it to `00176`, `swap`, one stack slot, or one global symbol.
- Stack-homed `load_global` publication now reads the prepared stack snapshot
  when available; watch for any future volatile/global reload semantics before
  broadening this beyond selected snapshot publication.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00130_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00195_c)$' | tee test_after.log`

Result: build was up to date; CTest ran 8 tests, 7 passed and 1 failed.
Passing: `backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00130_c`,
`c_testsuite_aarch64_backend_src_00176_c`, and
`c_testsuite_aarch64_backend_src_00195_c`. Failing:
`c_testsuite_aarch64_backend_src_00187_c` with the prior `RUNTIME_NONZERO`
segmentation fault. Canonical proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
