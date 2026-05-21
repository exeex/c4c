Status: Active
Source Idea Path: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Pointer-Derived Byte Load Lowering

# Current Packet

## Just Finished

Completed Plan Step 3 pointer-carrier reload repair in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`.

Code changes:

- Added a local helper that converts a tracked global address into runtime
  `PointerAddress` provenance rooted at the loaded SSA pointer value.
- Changed `try_lower_tracked_local_pointer_slot_load()` so
  `local_indirect_pointer_slots` reloads no longer republish fixed
  `global_pointer_slots[result]` provenance.
- Changed reloaded runtime pointer slots to publish the loaded SSA pointer as
  the new `PointerAddress` base with byte offset zero, instead of carrying
  stale base-plus-offset facts forward after store/reload cycles.

The new `backend_lir_to_bir_notes` coverage now passes: the incremented
string/global-backed pointer-carrier dereference lowers as a pointer-value load
rooted at `%reloaded.ptr` or equivalent `%advanced.ptr`, and no longer becomes a
fixed `.str0` `LoadGlobalInst`. Existing dynamic-global-scalar array coverage
in the same test remains green.

`00173` advanced from the prior runtime output-only failure (`copied string is`)
to a new runtime segmentation fault with empty stdout/stderr.

## Suggested Next

Localize the new `00173` segmentation fault now that pointer-carrier byte loads
are dynamic. The next coherent packet should compare the prepared BIR/AArch64
memory operations for the `*b`, `*src`, and destination-copy paths and identify
which dynamic pointer-value address is becoming invalid at runtime.

## Watchouts

- Do not special-case `00173`, one string literal, one global symbol, or one
  loop body.
- Keep direct fixed global/string byte loads distinct from dynamic pointer
  loads.
- Preserve existing dynamic-global-scalar array coverage that intentionally
  materializes global array element candidates with `LoadGlobalInst`; the
  failing owner is pointer-carrier dereference provenance, not every global
  scalar-array access.
- The semantic fixed-global-byte overfit is repaired; the live `00173` failure
  is now a runtime crash, so the next owner is likely downstream dynamic
  pointer-value address materialization or AArch64 memory lowering rather than
  the stale `global_pointer_slots` publication fixed here.
- Do not reopen AArch64 address-valued memory, recursive `00181` publication,
  runner behavior, expectations, timeout policy, or CTest registration.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`

Result: build completed; `backend_lir_to_bir_notes`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_memory_operand_contract` passed;
`c_testsuite_aarch64_backend_src_00173_c` failed with
`[RUNTIME_NONZERO]` / `exit=Segmentation fault` and empty stdout/stderr.
Proof log: `test_after.log`.
