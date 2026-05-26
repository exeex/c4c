# Current Packet

Status: Active
Source Idea Path: ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement One Non-Register Source-To-Stack Form

## Just Finished

Step 2 defined a target-local RISC-V scratch-register contract beside the
prepared edge-publication stack-destination consumer without broadening support.

Implemented contract:
- Current direct `Register -> StackSlot` stores reserve no scratch registers and
  clobber only the destination memory slot.
- Future I32 stack-destination source materialization may own `t0` as a value
  scratch only for one publication instruction sequence, clobbering it before
  the final `sw`; the scratch value must not live across publications.
- This consumer does not reserve `t1`/`t2`, and `t5`/`t6` remain available only
  to a later explicit address or large-offset helper contract.
- Non-register sources to `StackSlot` destinations remain fail-closed until a
  source form is implemented under that contract.

Focused tests still cover the unsupported `RematerializableImmediate`,
`StackSlot`, and `PointerBasePlusOffset` source-to-`StackSlot` forms; their
diagnostics now describe the remaining implementation gate instead of saying
the scratch policy is missing.

## Suggested Next

Next coherent packet: implement the narrow `RematerializableImmediate ->
StackSlot` I32 case using the local `t0` value-scratch contract, while keeping
`StackSlot -> StackSlot` and `PointerBasePlusOffset -> StackSlot` unsupported.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority
  for supported edge moves.
- The new scratch contract only covers value materialization through `t0` for
  one publication sequence; it does not define address scratch or large-offset
  stack-store expansion.
- Do not match fixture labels, value ids, stack slot ids, offsets, or test
  names.
- Existing `Register -> StackSlot` behavior must remain supported.
- Unsupported source or destination homes must remain explicit and fail closed.
- `StackSlot -> StackSlot` likely needs both load and store scratch/address
  policy, including the current source-size and load-offset restrictions.
- `PointerBasePlusOffset -> StackSlot` also needs pointer materialization and
  large-delta/aliasing policy; register-destination pointer support already
  rejects large deltas when destination aliases the base register.
- The current prepared-edge stack-destination path uses `sp` offsets, while
  broader `RiscvCodegen` helpers mostly use `s0` stack slots; do not mix those
  policies without confirming the prepared BIR frame convention.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`

Result: passed. Build completed after rebuilding the touched RISC-V emitter and
focused test binary; `test_after.log` records 1/1 focused test passing.
