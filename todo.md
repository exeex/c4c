Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared ABI/Home Path

# Current Packet

## Just Finished

Step 3 from `plan.md` repaired RV64 object-emission local frame-address
call-argument source publication for prepared register-source pointer
arguments.

- `fragment_for_prepared_call` now materializes an explicit prepared
  `LocalFrameAddressMaterialization` route into the selected GPR source before
  that source is consumed by a register call argument.
- `append_rv64_prepared_scalar_stack_call_argument` now applies the same
  prepared local frame-address publication before copying a register-source
  pointer into an outgoing stack argument, including active outgoing-stack
  adjustment in the frame offset.
- The fix still fails closed through the existing prepared frame-slot address
  authority checks when materialization facts are missing, incomplete,
  ambiguous, dynamic-frame-only, or out of range.
- Focused object-emission coverage now proves a complete local frame-address
  route feeding both a register argument and a stack argument, and converts the
  old incomplete address-provenance register-source case into rejection
  coverage.

## Suggested Next

Next packet should repair or reclassify the new `src/20001017-1.c` residual in
`bug`: the linked binary now reaches the callee and immediately branches to
`abort` after comparing `a0` against `ld t3, 0x58(sp)`, which is the saved
return-address slot in `bug`'s frame, not the incoming `Cref` formal value.

## Watchouts

- The local address helper intentionally reuses
  `prepared_frame_slot_address_call_argument_offset`, so missing prepared
  addressing/frame-plan authority remains fail-closed instead of falling back to
  source syntax or final assembly.
- Stack-argument local frame-address publication must account for an active
  outgoing call-stack adjustment; the focused test checks this with an adjusted
  `s2` materialization before storing the stack argument.
- The prior caller-side local frame-address residual is gone in the linked
  `main`: it now emits `addi t0, sp, 0x10`, adjusted `addi t0, sp, 0x28`,
  `addi s1, sp, 0x18`, and `addi s2, sp, 0x20` before consuming those pointer
  sources as register or stack call arguments.
- The new residual appears callee-side in `bug`, where the first comparison
  loads `0x58(sp)`, the saved `ra` slot, before comparing with `a0` and
  branching to `abort`.

## Proof

Ran the exact supervisor proof command into `test_after.log`:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Proof status: build passed; all six focused CTests passed; the one-row torture
probe still failed with `RV64_BACKEND_RUNTIME_MISMATCH`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`. The repaired caller-side local frame-address
source publication is visible in `main`; the fresh residual owner is callee-side
formal value/home materialization for `bug`'s `Cref` comparison, which currently
loads from the saved-RA stack slot. The exact proof log is `test_after.log`.
