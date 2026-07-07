Status: Active
Source Idea Path: ideas/open/575_rv64_pointer_arithmetic_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Pointer Arithmetic Shape

# Current Packet

## Just Finished

`plan.md` Step 1 classified the representative pointer arithmetic shape from
the existing 570 evidence without rerunning the route.

First semantic owner:
- Function/block: `foo`, `entry`
- Instruction: block index `0`, instruction index `7`, `BinaryInst`
- Operation: `%t4 = bir.add ptr %t1, %t4.byte_offset`
- Result owner/storage: `ptr %t4`, value id `7`, register-backed in `t0`
- Left operand: `%t1`, type `ptr`, loaded from `%lv.param.sp`, value id `3`,
  register-backed in `s1`
- Right operand: `%t4.byte_offset`, type `i64`, computed as
  `%t4.byte_offset = bir.mul i64 %t3, 4` after `%t3 = bir.sub i64 0, %t2`,
  value id `6`, register-backed in `s2`
- Prepared move shape before the owner: two register sources (`%t1` and
  `%t4.byte_offset`) fan into destination value `%t4`/`t0` with
  `authority=none` consumer register-to-register moves.
- Current diagnostic: `unsupported_instruction_fragment: BIR instruction
  requires unsupported RV64 object lowering; function=foo; block=entry;
  block_index=0; instruction_index=7; instruction_kind=BinaryInst; owner=ptr
  %t4`

The prepared-BIR representation is sufficient for RV64 object lowering: it is
a semantic pointer add of a loaded pointer base plus a scaled integer byte
offset, with all relevant values register-backed. No producer-side split is
recommended for this shape.

## Suggested Next

Proceed to Step 2 in RV64 object emission: add focused backend coverage for a
loaded pointer base plus scaled integer byte offset producing a pointer owner,
plus a specific fail-closed diagnostic for unsupported pointer arithmetic
forms.

## Watchouts

- Do not handle only `src/20000819-1.c`, `%t4`, or one exact prepared-BIR shape.
- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or runtime comparison changes.
- Keep unrelated instruction-fragment families out of this runbook.
- The existing representative failure is still the generic
  `unsupported_instruction_fragment`; Step 2 should encode the missing behavior
  or narrower diagnostic before implementation.

## Proof

Inspected the delegated 570 artifacts:
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/object-route.log`

No route rerun was needed because the saved evidence already names the first
owner, operand shape, storage, and current diagnostic. No `test_after.log` was
produced for this classification-only packet.
