Status: Active
Source Idea Path: ideas/open/575_rv64_pointer_arithmetic_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement RV64 Pointer Arithmetic Lowering

# Current Packet

## Just Finished

`plan.md` Step 3 added a semantic RV64 object-emission diagnostic for the
unsupported loaded-pointer-base plus scaled-integer-byte-offset arithmetic shape
covered by Step 2, without implementing pointer-add lowering yet.

The diagnostic recognizes pointer-result add/sub forms only when the pointer
operand is produced by an earlier `LoadLocalInst` in the same block and the
integer offset is an immediate or an earlier `Mul` producer. It still lets
supported prepared pointer/frame address materialization and scalar binary
fragments win before reporting:
`unsupported_pointer_arithmetic: RV64 object route requires prepared pointer
arithmetic lowering for loaded pointer base plus scaled integer byte offset`.

The focused test now passes and unrelated unsupported binary forms, including
the existing dynamic pointer-result frame-address fail-closed cases, remain on
the generic `unsupported_instruction_fragment` diagnostic.

## Suggested Next

Proceed to the next Step 3 slice only if the supervisor wants actual pointer-add
lowering now: implement the semantic RV64 lowering for the same loaded pointer
base plus scaled integer byte offset shape while preserving owner publication.

## Watchouts

- This slice intentionally stops at the semantic diagnostic; it does not emit
  the pointer add instruction sequence.
- Keep the diagnostic classification tied to producer/value semantics, not
  fixture/function/value spellings.
- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or runtime comparison changes.
- When adding real lowering, preserve the result owner publication path for
  register and stack homes.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` is the preserved proof log for this packet.
