# Current Packet

Status: Active
Source Idea Path: ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement One Non-Register Source-To-Stack Form

## Just Finished

Step 3 implemented the narrow `RematerializableImmediate -> StackSlot` I32
prepared edge-publication form through shared `edge_publications` lookup
authority.

Implemented behavior:
- `RematerializableImmediate -> StackSlot` emits `li t0, imm` followed by
  `sw t0, offset(sp)` for concrete 4-byte stack destinations with direct
  signed 12-bit `sp` offsets.
- Existing `Register -> StackSlot` stores remain supported.
- `StackSlot -> StackSlot` and `PointerBasePlusOffset -> StackSlot` remain
  explicit fail-closed forms.
- Malformed destinations, unsupported destination sizes, non-move publications,
  missing shared publication facts, and large stack-destination offsets remain
  fail closed.
- Focused tests now cover the new immediate-to-stack support, preservation of
  shared publication authority, existing register-source stack stores, and the
  remaining unsupported neighboring forms.

## Suggested Next

Next coherent packet: Step 5 validation/decision pass to decide whether this
runbook can close or whether remaining `StackSlot -> StackSlot` and
`PointerBasePlusOffset -> StackSlot` policies need follow-up source ideas.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority
  for supported edge moves.
- The implemented scratch contract only covers immediate value materialization
  through `t0` for one publication sequence; it does not define address scratch
  or large-offset stack-store expansion.
- Do not match fixture labels, value ids, stack slot ids, offsets, or test
  names.
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
