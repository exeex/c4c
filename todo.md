Status: Active
Source Idea Path: ideas/open/666_rv64_callee_saved_gpr_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Callee-Saved And Live-Value Evidence

# Current Packet

## Just Finished

Step 1 refreshed rows 183, 184, and 219 together. The focused proof reproduced
the current boundary: CLI/codegen object-data rows pass, while the three RV64
object-runtime rows fail under QEMU (`row183 exit=224 expected=11`,
`row184 exit=224 expected=24`, `row219 exit=1 expected=0`).

Rows 183 and 184 still preserve the retired 663 finding. Row 183 emits a
4-byte `.bss` object symbol
`__static_local_rv64_step3_static_counter_0` plus PC-relative load/store
relocations, and row 184 emits a 4-byte `.data` object symbol
`__static_local_rv64_step4_static_initialized_counter_0` with payload
`0d000000` plus the same relocation shape. Their prepared object-data,
static-storage layout, initializer payload, symbol, and relocation facts are
not the first owner.

Prepared call facts for all three rows publish the call result from `a0` to
caller-saved `t0`, model `t0`/`a0` as clobbered, keep `s1`/`s2` out of the
call-clobber set, assign fixed callee-saved save slots, and emit matching
prologue/epilogue save/restore in the object route. The runtime corruption
appears when RV64 object-route text consumes the callee-saved preservation fact
after the call by moving the preserved value back into `t0` before the
fresh call result has been stored or consumed:

- row 183: after `rv64_step3_static_counter`, object text does
  `mv t0,a0; mv t0,s2; sw t1,0(sp)` and repeats the same stale `s2`
  overwrite before storing the second result.
- row 184: same pattern after `rv64_step4_static_initialized_counter`.
- row 219: `keep_value_across_call` does `mv t0,a0; mv t0,s2; sw t1,4(sp)`,
  and `main` does `mv t0,a0; mv t0,s1; sw t1,0(sp)`, so the result path
  compares a stale preserved value instead of `37`.

First owner: one shared RV64 object-route live-value consumption/order owner.
No split is justified yet. The failing edge is not live-range publication,
callee-saved slot placement, save/restore emission, or call clobber modeling;
it is the object-route consumer restoring a preserved callee-saved value into
the caller-saved result register at the wrong point.

## Suggested Next

Execute Step 2 by selecting the shared RV64 object-route live-value
consumption/order boundary for rows 183, 184, and 219. Define the repair packet
around preserving the call result publication before any callee-saved
preservation restore can overwrite the result register, with fail-closed
diagnostics for missing or ambiguous call-result/preserved-value ordering
facts.

## Watchouts

- Keep byval payloads, pointer-local lowering, static-storage object-data
  publication/layout/initializer/relocation repair, packed local member
  offsets, CLI dump formatting, AArch64 dispatch, generic RISC-V object
  emission, and LLVM torture work outside this plan unless focused evidence
  proves the same first owner.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or unrelated backend families.
- Reject fixed-register or named-row shortcuts; repair one general
  callee-saved/live-value rule only after the first owner is proven.
- `c4c-objdump` rejects these relocatable objects because of unsupported text
  relocations and rejects linked binaries because they are not relocatable
  objects; use `riscv64-linux-gnu-objdump` for this focused linked/object
  disassembly evidence.
- The assembly-route text for row 219 lacks the stale post-call `mv t0,s2`
  overwrite that appears in object-route disassembly, so keep Step 2 focused
  on object-route consumption/order rather than broad call lowering.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_prepared_object_data_static_local_storage_obj|backend_cli_riscv64_prepared_object_data_static_local_initialized_storage_obj|backend_codegen_route_riscv64_prepared_object_data_static_local_storage|backend_codegen_route_riscv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_callee_saved_gpr_live_across_call)$' > test_after.log 2>&1`

Result: build passed, focused CTest returned the expected failing evidence with
4 passed and 3 failed out of 7. Passing rows were the two CLI object-data tests
and the two codegen-route object-data tests. Failing rows were exactly 183,
184, and 219. Proof log: `test_after.log`.

Supplementary read-only diagnostics used for ownership evidence:
`build/c4cll --dump-prepared-bir --target riscv64-linux-gnu` on all three
focused case sources, `riscv64-linux-gnu-objdump -dr` on the focused runtime
objects, `riscv64-linux-gnu-objdump -d` on the focused linked binaries,
`riscv64-linux-gnu-readelf -S -s -r -x .data` for rows 183/184, and direct
`qemu-riscv64 -L /usr/riscv64-linux-gnu` runtime checks for the three linked
binaries.
