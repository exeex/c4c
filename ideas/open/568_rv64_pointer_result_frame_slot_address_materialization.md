# RV64 Pointer-Result Frame-Slot Address Materialization

Status: Open
Type: RV64 object-lowering implementation follow-up
Parent: `ideas/closed/567_rv64_integer_div_rem_instruction_fragment_lowering.md`
Owning Layer: RV64/MIR prepared object lowering

## Goal

Lower prepared pointer-result local address materialization for dynamic
frame-slot/aggregate element addresses in RV64 object emission, starting from
the pinned `bir.add ptr` fragment in `src/20001026-1.c`.

## Why This Exists

Idea 567 proved that the representative residual originally routed through
`integer_div_rem` is not missing div/rem opcode support. The first unsupported
instruction after existing div/rem lowering is:

```text
%t12 = bir.add ptr %lv.r.0, %t12.byte_offset
```

Pinned evidence:

- Artifact: `build/agent_state/567_step2_pinned_fragment.md`
- Case: `src/20001026-1.c`
- Function/block: `real_value_from_int_cst`, `block_1`
- Traversal location: block index `3`, instruction index `4`
- `%lv.r.0`: value id `14`, GPR register home `s1`, placement
  `gpr:callee_saved#0/w1`
- `%t12.byte_offset`: value id `12`, frame-slot home `slot#22+stack80`
- `%t12`: value id `13`, pointer frame-slot home `slot#23+stack88`
- Prepared address-materialization fact for the same source block and
  instruction index: `kind=frame_slot`, `result=%lv.r.0`, `offset=8`

Current object emission falls through
`fragment_for_prepared_instruction(...)` after the existing frame-address,
symbol-address, and integer binary hooks do not claim the pointer-result
binary instruction.

## In Scope

- Define the semantic contract for RV64 prepared pointer-result address
  materialization where a pointer base and integer byte offset produce a
  pointer result.
- Reuse existing prepared address-materialization facts and value-home
  authority rather than matching source testcase names or instruction text.
- Materialize the result into the prepared destination home, including
  frame-slot destinations, using existing RV64 move/publication conventions.
- Add focused backend/object-emission coverage for the pinned shape, including
  fail-closed behavior for missing or incoherent prepared facts.
- Prove the representative allowlist containing `src/20001026-1.c` advances
  past the pinned `bir.add ptr` unsupported fragment, with any new residual
  diagnostic recorded as a downstream owner.

## Out Of Scope

- Adding or changing raw `sdiv`, `udiv`, `srem`, or `urem` opcode lowering.
- Broad pointer/integer cast cleanup outside the pinned pointer-result address
  materialization shape.
- F128, scalar FPR, ABI/call, runtime, or producer/prepared admission policy.
- Rewriting pass/fail expectations, unsupported markers, or allowlists as a
  substitute for lowering support.
- General local-memory redesign beyond what is required to consume the
  prepared address-materialization facts for this pointer-result fragment.

## Acceptance Criteria

- Focused tests cover pointer-result frame-slot/local-address materialization
  from a pointer base plus dynamic integer byte offset.
- The implementation consumes semantic prepared facts and value homes; it does
  not match `src/20001026-1.c`, `%t12`, diagnostic strings, or raw text shapes.
- Malformed or missing prepared facts fail closed with existing unsupported
  behavior.
- `cmake --build --preset default` and the supervisor-selected backend subset
  pass.
- A representative allowlist containing `src/20001026-1.c` no longer reports
  the pinned generic unsupported fragment for
  `%t12 = bir.add ptr %lv.r.0, %t12.byte_offset`; any new residual is recorded
  with a concrete downstream owner.

## Reviewer Reject Signals

- Reject duplicate div/rem opcode lowering, div/rem test duplication, or claims
  of div/rem progress as the repair for this idea.
- Reject testcase-name dispatch, `%t12`-specific matching, instruction-text
  string matching, or diagnostic-string matching.
- Reject a patch that handles only constant offsets if the pinned dynamic byte
  offset still falls through generic object emission.
- Reject result publication that ignores the prepared destination home or
  silently drops a pointer result stored in a frame slot.
- Reject broad pointer/local-memory rewrites that are not required to lower the
  pinned prepared pointer-result address-materialization contract.
- Reject expectation rewrites, unsupported downgrades, or weaker test
  contracts without explicit user approval.
