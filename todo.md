# Current Packet

Status: Active
Source Idea Path: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority tuple

## Just Finished

- Step 1 selected exactly one new row: an unchanged current-function
  `LirNativeBodyParameterAbi::DirectScalar` integer parameter used directly as
  `LirSwitch.selector`. The native tuple is the parameter-definition
  `LirValueId`, `LirFunction.link_name_id` owner, parameter index,
  `LirTypeRef`, `DirectScalar` ABI, new `SwitchSelector` role, and exact
  `LirSwitch.selector`/`selector_type_ref` consuming relation.
- The existing structured producer seam is
  `StmtEmitter::emit_control_flow_stmt(const SwitchStmt&)`; the selected
  schema/verifier seam is `LirSwitch` plus `verify_switch_selector`. Do not
  reuse `LirBinOp.scalar_lhs_parameter_authority` or its materializing add:
  that is the already accepted binary-LHS row, not a direct switch-selector
  relation.

## Suggested Next

- Execute Step 2 only: add a dedicated optional switch-selector authority
  carrier and `SwitchSelector` role; produce it directly from the native
  parameter definition and verify unique same-function owner/index/type/ABI/
  role plus exact `LirSwitch.selector` and `selector_type_ref` coherence.

## Watchouts

- Do not modify Raw-BIR/importer code or select semantic authority from text,
  names, signatures, rendered operands, printer output, or testcase shape.
- All nonselected parameter forms, and the existing binary-LHS/RHS/return and
  direct-pointer rows, remain fail closed. The four prior 734 rows do not
  reopen.

## Proof

- Step 1 was selection-only; no build/test was required. Step 2 must define
  focused nearby producer positive/negative proof before publication.
