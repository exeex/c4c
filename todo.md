Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Trace Destination And Updated-Pointer Facts

# Current Packet

## Just Finished

Step 2 traced the callee destination and updated-pointer facts from semantic
BIR through prepared value homes and added focused dump coverage under
`tests/backend/case/riscv64_indirect_store_postincrement_callee_contract.c`.

Focused contract:

- Callee source pointer: `%p.base`.
- Old caller-provided destination pointer:
  `%t9 = bir.load_local ptr %lv.param.cursor`.
- Postincremented local pointer writeback:
  `%t10 = bir.add ptr %t9, 8` followed by
  `bir.store_local %lv.param.cursor, ptr %t10`.
- Caller-visible indirect store:
  `bir.store_local %t9.store.addr, ptr %t8, addr %t9`.

Prepared facts:

- `%t9` has a register home (`s1`), so the old pointer value is available as
  the indirect-store destination.
- `%t10` has a stack-slot home and a selected binary producer/freshness fact:
  `store_source function=writeback_callee block=block_1 inst=6 source=%t10
  ... source_producer=binary ... source_freshness_status=selected`.
- `%t8` has a selected binary producer/freshness fact:
  `store_source function=writeback_callee block=block_1 inst=7 source=%t8
  ... source_producer=binary ... source_freshness_status=selected`.
- Prepared addressing distinguishes the local pointer-variable writeback from
  the caller-visible store:
  `access block=block_1 inst_index=6 base=frame_slot stored=%t10 frame_slot=#0`
  and
  `access block=block_1 inst_index=7 base=pointer_value stored=%t8 pointer=%t9`.

Classification:

- There is a positive producer/consumer contract for both the local
  postincrement writeback and the indirect store.
- The remaining owner is RV64 object lowering for prepared pointer-value
  stores: it must consume `base=pointer_value stored=%t8 pointer=%t9` as a
  memory store through the old pointer value, while preserving the separate
  frame-slot store to the local cursor home.
- The GCC torture `loop-2e.c` row remains representative evidence for the same
  failure, not the only proof surface.

## Suggested Next

Execute Step 3: repair the general RV64 prepared pointer-value store lowering
for the proven contract. The focused legal shape should emit the indirect
store through the old pointer value (`pointer=%t9`) instead of reusing the
frame-slot destination for the local cursor writeback.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- Step 2 found no missing semantic or prepared producer fact for the callee
  indirect store/writeback shape; an implementation slice should not infer
  destination authority from runtime behavior, source names, or final assembly.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not special-case `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.

## Proof

Focused dump contract:

`ctest --test-dir build --output-on-failure -R '^backend_dump_riscv64_indirect_store_postincrement_callee_contract$'`

Result: pass. Log:
`build/agent_state/657_step2_destination_writeback_facts/focused_dump_ctest.log`.

Step 2 evidence:

- `build/agent_state/657_step2_destination_writeback_facts/indirect_store_postincrement_callee.prepared.txt`
- `build/agent_state/657_step2_destination_writeback_facts/summary.md`

Delegated proof run exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: CTest exited `8`; `test_after.log` reports
`91% tests passed, 32 tests failed out of 367`.
