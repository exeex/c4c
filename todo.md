# Current Packet

Status: Active
Source Idea Path: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the justified unary fneg authority repair

## Just Finished

- Completed plan Step 1 trace. In `lir_floating_unary_minus_ternary_phi_authority`, the `-left` arm lowers in `expr/misc.cpp` to `LirBinOp{fresh_value, "fneg", LirTypeRef("double"), lhs=%p.left, rhs=empty}`. `%p.left` carries exactly `ctx.param_value_authorities[1]`, equal to the current function's unique native definition `{value, parameter_index=1, type=LirTypeRef("double"), owner=function.link_name_id, abi=DirectScalar}`; the owner is the `lir_floating_unary_minus_ternary_phi_authority` function and the tuple role is `Lhs`. `verify_scalar_binary_lhs_authority` already detects that same direct-scalar lhs and requires/exactly mirrors the left tuple, including `op.type_str == authority.type`; unary lowering simply does not publish it.

## Suggested Next

- Execute Step 2 only: in the unary floating-minus `fneg` branch, look up the current-function native definition by structured lhs value, exact `LirTypeRef`, current owner, and `DirectScalar`, then publish the existing `LirScalarBinaryLhsParameterAuthority` with role `Lhs` into that `LirBinOp`. Keep the verifier tuple relation; add the minimal structural coherence that `FNeg` has an empty rhs (the existing reverse check already rejects empty rhs for all non-`FNeg` opcodes). Cover the positive tuple and malformed omitted/mismatched tuple for unary `fneg`; do not infer any fact from rendered spellings or enter binary, selector, Raw-BIR/importer, generic, or non-unary-fneg rows.

## Watchouts

- Preserve the dirty 821/822/825-related worktree changes. Do not broaden into binary producers, switch selectors, Raw-BIR/importer, or generic rows. The parent composite CTest is diagnostic only.
- The existing scalar-lhs schema is structurally sufficient: its operand position is the unary `fneg` operand, and its value/index/type/owner/ABI/role are independently verified against `native_body_parameter_definitions`. It needs no schema redesign or new authority class; Step 2 must retain fail-closed opcode/shape coherence.

## Proof

- Diagnosis packet only; no build, test subset, or canonical regression log was run or created. Select Step 2's focused proof after its implementation scope is delegated.
