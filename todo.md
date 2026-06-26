Status: Active
Source Idea Path: ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Rejected Move Bundle

# Current Packet

## Just Finished

Completed Step 1: audited the `src/20000217-1.c` prepared move-bundle
rejection behind
`unsupported_move_bundle_target_shape: prepared move bundle requires unsupported
RV64 moves`.

The current failure is downstream of idea 375 because the regenerated prepared
BIR has `%t11 = bir.trunc i32 %t9 to i16` after `%t9 = bir.sge i32 %t8, 8`.
The first move-bundle rejection in prepared traversal order is not the later
compare/trunc publication. It is the before-instruction bundle at
`showbug` block 0, instruction 1:

- phase: `before_instruction`
- bundle cardinality: 1 move
- move: `from_value_id=2` (`%t0`) to `to_value_id=3` (`%t1`)
- source home: scalar integer stack slot, `%t0`, `i16`, slot `#6`, stack offset
  `8`
- destination home: scalar integer GPR value home, `%t1`, register `s1`
- destination kind/storage: `value` / `register`
- prepared lane width: `destination_contiguous_width = 1`
- operation: `op_kind=move`, `uses_cycle_temp_source=no`
- reason: `consumer_stack_to_register`
- rejection site:
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `fragment_for_prepared_move_bundle()`, after the helper accepts only
  register-destination bundles but cannot obtain a GPR/FPR register source from
  a stack-slot source home.

First in-scope scalar integer move target shape: prepared
before-instruction value move from a scalar integer stack-slot home to a scalar
integer GPR value home, proven from prepared metadata rather than testcase
names, value IDs, instruction indexes, or fixed registers.

Audit artifacts:
- `build/agent_state/376_step1_prepared_move_bundle_audit.txt`
- `build/agent_state/376_step1_20000217-1.showbug-semantic-bir.txt`
- `build/agent_state/376_step1_20000217-1.showbug-prepared-bir.txt`
- `build/agent_state/376_step1_20000217-1.codegen-obj.log`

## Suggested Next

Execute Step 2 from `plan.md`: add focused RV64 object-emission tests for a
prepared scalar integer stack-slot to GPR value move bundle, plus nearby
fail-closed tests for unsupported stack-slot destinations and cycle-temp or
multi-lane variants.

## Watchouts

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not infer move semantics from testcase names, hard-coded value ids, source
  syntax, instruction indexes, or the exact representative expression.
- Keep unsupported homes, widths, phases, and ambiguous bundles fail-closed with
  precise diagnostics.
- Adjacent shapes visible in `showbug` but not selected as the first packet:
  register-to-stack value moves at instruction 5, stack-to-stack value move at
  instruction 6, the compare/trunc register-to-stack publication at instruction
  11, and the before-return stack-slot to `a0` return-ABI move at instruction
  12.

## Proof

Audit-only packet; no build proof required. Regenerated focused dumps and the
expected object-route failure under `build/agent_state/376_step1_*`; no
root-level logs were written.
