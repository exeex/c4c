Status: Active
Source Idea Path: ideas/open/314_aarch64_large_stack_offset_addressing.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 classification for idea 314 reran the focused proof and classified the
two post-Step-2 first bad facts without implementation or test changes.

`00216.c` is no longer failing on illegal AArch64 stack-memory instruction
spelling. The prior first bad fact `ldr x13, [sp, #1644]` has been legalized in
`build/c_testsuite_aarch64_backend/src/00216.c.s` as:

```asm
add x9, sp, #1644
ldr x13, [x9]
```

The remaining failure is a runtime output mismatch. Generated-code evidence
points to a separate frame-slot/frame-layout consistency owner rather than the
idea-314 instruction-encoding owner: `test_correct_filling` reserves only
48 bytes with `sub sp, sp, #48`, but the same function still emits accesses
through `sp + 1600`, `sp + 1624`, `sp + 1644`, and `sp + 1648`. The new first
bad fact is therefore "legalized accesses target offsets outside the function
frame", not "large stack-slot load/store or stack-backed scalar publication
cannot be legally spelled".

`00204.c` is no longer failing on scalar ALU stack publication. The remaining
failure is a frame prologue printer rejection:

```text
cannot print AArch64 machine node family=frame opcode=frame_setup:
frame adjustment immediate is outside the plain #imm encoding range 0..4095
```

The prepared dump for `stdarg` reports `frame_size=5776`, and
`src/backend/mir/aarch64/codegen/machine_printer.cpp::print_frame` currently
rejects `frame.frame_size_bytes > 4095` before printing `sub sp, sp, #...`.
That is a frame setup/teardown materialization gap, not a stack-slot load/store
or stack-backed scalar publication gap.

## Suggested Next

Ask the plan owner to decide lifecycle handling for idea 314 with the Step 4
classification evidence. Suggested action: do not expand idea 314. Close or
retire it as having completed the large stack-offset instruction-spelling
owner, then open/switch to separate lifecycle work for:

- AArch64 frame setup/teardown materialization when frame size exceeds the
  plain `sub/add sp, sp, #imm` range (`00204.c` representative).
- AArch64 frame-slot/frame-layout consistency when generated stack slot offsets
  exceed the actual function frame allocation (`00216.c` representative).

## Watchouts

- Do not fold the `00204.c` frame prologue immediate into idea 314 without a
  lifecycle decision. It is a frame instruction owner with different correctness
  and coverage needs from selected stack-slot memory operands.
- Do not treat the `00216.c` runtime mismatch as generic runtime fallout. The
  concrete generated-code clue is a function frame-size/slot-offset mismatch:
  large offsets are now legal to spell, but they do not fit the actual frame
  reserved by the prologue.
- No expectations, unsupported classifications, runners, CTest registration,
  timeout policy, proof-log policy, implementation files, or tests were changed
  in this packet.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_002(04|16)_c)$'`

Result recorded in `test_after.log`: build succeeded; CTest ran 12 tests with
10 passing and the same two classified residuals failing. The passing set
includes `backend_lir_to_bir_notes`,
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
the prior 00204 semantic/prepared/focused dump guardrails.

Remaining failures:

- `c_testsuite_aarch64_backend_src_00216_c`: `[RUNTIME_MISMATCH]`; classified
  outside idea 314's instruction-spelling owner and suggested for a
  frame-slot/frame-layout consistency owner.
- `c_testsuite_aarch64_backend_src_00204_c`: `[FRONTEND_FAIL]` at
  `family=frame opcode=frame_setup`; classified outside idea 314's
  load/store/scalar-publication owner and suggested for a frame adjustment
  materialization owner.
