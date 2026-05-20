Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Fallthrough Fixed-Offset Local Load Store Emission

# Current Packet

## Just Finished

Step 2 repaired fallthrough fixed-offset local load/store emission by letting
AArch64 prepared block dispatch lower memory instructions through the prepared
memory-access index when retained BIR instruction indices do not have a matching
prepared access. The repair is width-general and is covered by a focused
backend dispatch fixture that places an i16 fixed-offset local load/store copy
inside a branch block after unrelated scalar operations; the fixture now prints
`ldrh w10, [sp, #18]` and `strh w10, [sp, #34]` from selected memory nodes.

Regenerated `/tmp/c4c_00143_after.s` confirms the external representative now
contains the previously missing fallthrough data-copy memory movement. For
example, the path around `.LBB1_27` now emits `ldrh w9, [sp]`,
`strh w9, [sp, #616]`, `ldrh w9, [sp, #616]`, and `strh w9, [sp, #78]`
between the pointer-local updates and the branch onward. The remaining `00143`
failure is therefore classified as a new residual after fixed-offset local copy
emission, not as continued loss of the fallthrough load/store nodes.

## Suggested Next

Investigate the post-emission `00143` runtime residual by comparing the newly
complete fallthrough copy assembly against expected Duff-device control/data
flow. Keep the next packet separate from fixed-offset load/store survival unless
the first bad fact still points back to prepared memory emission.

## Watchouts

- The repair intentionally preserves the existing first-memory-instruction
  diagnostic behavior for index zero and keeps the older byte-only retry path;
  it only switches to the prepared memory index when the raw retained BIR index
  has no prepared access and the prepared memory index does.
- `src/backend/mir/aarch64/codegen/memory.cpp` did not need a lowering change;
  the selected memory records and printer were already able to emit the stack
  `ldrh`/`strh` once dispatch supplied the prepared access index.
- Do not classify future `00143` progress solely from the external test; keep
  focused backend evidence for whatever residual is found next.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`.
The build succeeded; `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, `00086`, and `00111` passed. `00143` still
fails `[RUNTIME_NONZERO]` with exit 1 after the fallthrough fixed-offset
`ldrh`/`strh` copies are present in regenerated assembly. Proof log path:
`test_after.log`.
