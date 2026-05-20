Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fallthrough Load Store Drop Boundary

# Current Packet

## Just Finished

Step 1 localized the fallthrough fixed-offset local load/store drop boundary.
Regenerated `/tmp/c4c_00143_prepared_bir.txt` shows `block_9` through
`block_15` still contain the fixed-offset local data-copy operations, e.g.
`%t40 = bir.load_local i16 %t40.addr, addr %lv.a.0+2` followed by
`bir.store_local %lv.b.0, i16 %t40, addr %lv.b.0+2`, and
`--- prepared-atomic-operations ---` records matching prepared memory accesses
for those block/instruction pairs. Regenerated `/tmp/c4c_00143.s` shows the
corresponding AArch64 fallthrough labels only materialize/update `from`/`to`
pointer locals and branch onward; the prior non-fallthrough `block_6` copy still
prints `ldrh`/`strh`, so the loss is after prepared BIR/addressing and before or
inside AArch64 machine-node emission for the fallthrough fixed-offset
`LoadLocalInst`/`StoreLocalInst` pair.

Concrete first bad owner: AArch64 prepared block dispatch/memory emission,
centered on `dispatch_prepared_block` in
`src/backend/mir/aarch64/codegen/dispatch.cpp` handing the fixed-offset local
load/store instructions to `lower_memory_instruction` in
`src/backend/mir/aarch64/codegen/memory.cpp`. The prepared memory access table is
still correct, and the final printer can emit stack-slot `ldrh`/`strh` for the
same shape in `block_6`, but the fallthrough block machine output reaches
`machine_printer.cpp` without the data-copy memory nodes.

## Suggested Next

Execute Step 2 by repairing the general AArch64 prepared block
dispatch/memory-emission path so fixed-offset local `LoadLocalInst` and
`StoreLocalInst` operations in branch/fallthrough blocks survive as selected
memory machine nodes. Start with a focused trace or instrumentation around
`dispatch_prepared_block` calls to `lower_memory_instruction` for prepared
frame-slot accesses; the repair should preserve the existing ordinary
stack-slot load publication and stack-slot store-source printing used by the
`block_6` copy, without matching `00143`, block labels, local names, or emitted
instruction strings.

## Watchouts

- `/tmp/c4c_00143_mir.txt` and `/tmp/c4c_00143_trace_mir.txt` currently expose
  only the x86/debug summary surface for an AArch64 target, so they do not prove
  selected-node presence/absence.
- The useful regenerated probes are:
  `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c > /tmp/c4c_00143_prepared_bir.txt 2>/tmp/c4c_00143_prepared_bir.err`
  and
  `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s > /tmp/c4c_00143_asm.stdout 2>/tmp/c4c_00143_asm.stderr`.
- Do not repair this by special-casing `00143`, Duff-device labels, block
  numbers, local names, source lines, emitted instruction text, expectations,
  CTest registration, runner behavior, or timeout/proof policy.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll -j 2 && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > test_after.log 2>&1`.
The build was up to date and the focused CTest reproduced the known
`[RUNTIME_NONZERO]` residual for `00143`; proof log path: `test_after.log`.
