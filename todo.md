# Current Packet

Status: Active
Source Idea Path: ideas/open/829_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the selected authority

## Just Finished

- Step 1 traced and selected exactly one next row: a DirectScalar
  current-function parameter used unchanged as `LirCallOp.structured_args[1]`
  of a direct, non-variadic, specified call. Its source tuple is the existing
  `LirCurrentFunctionBodyParameterDefinition` for that second source parameter
  (`value`, current-function `LinkNameId` owner, `parameter_index = 1`, exact
  `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`); its required
  consumer coherence is `structured_args[1].operand == value`, equal argument,
  `arg_type_refs[1]`, and fixed-callee parameter-1 types. Native construction
  currently allocates this definition in `hir_to_lir.cpp`, but
  `publish_fixed_direct_call_argument0_authority` and its verifier inspect
  index 0 only, so this valid index-1 relation has no structured carrier and
  cannot be recovered from display operands/signatures.

## Suggested Next

- Step 2 only: add an index-1-specific call-argument authority carrier and
  `FixedDirectCallArgument1` role; publish it from the existing parameter
  definition and verify the exact direct/non-variadic/specified,
  SSA/value-owner-index-type-ABI-role, and parameter-1 type-coherence rules.
  Add one two-parameter direct-call positive fixture plus missing, invalid,
  duplicate, foreign, owner/index/type/ABI/role, and consumer-incoherent
  malformed cases. Proposed files: `src/codegen/lir/ir.hpp`,
  `src/codegen/lir/hir_to_lir/call/target.cpp`, `src/codegen/lir/verify.cpp`,
  and `tests/frontend/frontend_lir_call_type_ref_test.cpp`.

## Watchouts

- Do not change Raw-BIR/importer code, reopen 734's accepted rows, or infer
  semantic authority from presentation fields.
- Keep all other call indices, indirect/variadic/unspecified calls, converted
  arguments, and every other parameter form fail closed; this is not generic
  call-argument parameter authority.
- Preserve unrelated 821/822 worktree material.

## Proof

- No 829 proof yet. Step 2 requires a fresh build and exact focused producer
  proof selected from the traced semantic relation.
