# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Receive the checked i32 output-only inline-assembly binding

## Just Finished

- Plan Step 5.3.3 received only the checked downstream `double` `LirBinOp`
  `FAdd`: one typed Raw-BIR `Binary` with an `F64` source-backed result,
  ordered current-function SSA operands, and a left edge restricted to the
  accepted direct native-double Call result. Raw and Canonical receipt plus
  missing, duplicate, cross-owner, wrong-type, non-`FAdd`, non-SSA, and
  malformed-result rollback coverage now pass.

## Suggested Next

- Implement only Plan Step 6.1: map the producer-verified
  `ordinary_results[0]` i32/`Output`/index-zero `LirValueId` into the existing
  Raw-BIR `InlineAsmNode` result and register it by that native source ID for
  its one same-typed Store use. Do not use compatibility spelling or opaque
  payload as value authority.

## Watchouts

- The new Binary container is deliberately closed to `FAdd`/`F64` and requires
  a direct native-double Call producer on the left. All other binary forms,
  literals, presentation-derived operands, casts, compares, selects, returns,
  and downstream uses remain fail-closed.
- Branch/conditional/switch target labels remain raw text and are not a
  Step-6 receiver row. For the new inline-asm source-ID receipt path, i64,
  inputs, read/write/tied, memory/address/immediate, clobber/explicit-register,
  vector/aggregate/multi-output, `insn_r`, and opaque-text-derived facts remain
  fail-closed. Preserve the existing closed idea-731 carrier rather than
  interpreting any target or constraint meaning.

## Proof

- Plan Step 5.3.3 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
  The focused subset passed; `test_after.log` is the proof log.
- Plan Step 6.1 must run a fresh build and the same focused receipt selection:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.
