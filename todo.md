# Current Packet

Status: Active
Source Idea Path: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority tuple

## Just Finished

- Step 1 selected exactly one new row: an unchanged current-function
  `LirNativeBodyParameterAbi::DirectScalar` integer parameter used directly as
  `LirSwitch.selector`. Idea 826's completed isolation-only prerequisite
  removed the shared-worktree collision without accepting any authority route.

## Suggested Next

- Execute Step 2 only: add a dedicated optional switch-selector authority
  carrier and `SwitchSelector` role; produce it directly from the native
  parameter definition and verify unique same-function owner/index/type/ABI/
  role plus exact `LirSwitch.selector` and `selector_type_ref` coherence.

## Watchouts

- The preserved 822 patch is outside this route. Do not restore, modify, or
  accept it; do not reuse `scalar_lhs_parameter_authority` or materialize an
  add.
- Do not modify Raw-BIR/importer code or select semantic authority from text,
  names, signatures, rendered operands, printer output, or testcase shape.
- All nonselected parameter forms, and the existing binary-LHS/RHS/return and
  direct-pointer rows, remain fail closed. The four prior 734 rows do not
  reopen.

## Proof

- Step 1 was selection-only. Step 2 must define focused nearby producer
  positive/negative proof before publication; the 826 focused 0/1 result is
  isolation evidence only and is not Step 2 acceptance proof.
