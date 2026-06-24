# RV64 Inline Asm Custom Vector Integration Review

## Goal

Perform the final integration review child for
`ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.

This child decides whether the closed scalar `.insn`, vector register
constraint, EV `.insn.d`, and consteval/template-string stages compose into the
intended C++ custom vector instruction workflow without reopening broad
implementation work.

## Why This Exists

The umbrella should close only after the staged children are completed and
reviewed together. The individual children prove their own surfaces, but the
umbrella promise is the composed route:

- user/library code supplies the custom instruction vocabulary;
- c4c owns inline asm parsing, constant template folding, register constraints,
  allocation, substitution, byte encoding, object emission, and side-effect
  semantics;
- no EV operation must become a compiler-known mnemonic or intrinsic.

This child is the final checkpoint before the parent umbrella can be closed or
split again for any real gap discovered during review.

## In Scope

- Review the closed child evidence for:
  - scalar RV64 `.insn r` object route:
    `ideas/closed/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
  - vector register constraints:
    `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`
  - EV 64-bit `.insn.d` route:
    `ideas/closed/342_rv64_ev_insn_d_inline_asm_stage3.md`
  - consteval/template inline asm strings:
    `ideas/closed/343_rv64_consteval_inline_asm_template_strings.md`
- Verify the current codebase still has representative tests or fixtures for
  the composed route from compile-time asm template through RV64 object bytes.
- Confirm `VR`, `VRM2`, and `VRM4` group allocation, overlap rules, and
  base-register substitution remain compatible with `.insn.d` encoding.
- Confirm literal and compile-time-generated `.insn.d` templates share the same
  lowering, metadata, substitution, and object emission path.
- Run a closure-quality validation subset selected by the supervisor.
- Record whether the parent umbrella is ready for close review or needs a
  focused follow-up child idea.

## Out Of Scope

- Adding new EV operation mnemonics or intrinsics.
- Broad RVV semantic lowering outside inline asm constraints.
- Named GNU operands, `%c[...]` modifiers, mask-specific constraints, or
  broader GNU assembler compatibility unless review proves one is required for
  the umbrella's already-stated acceptance criteria.
- Reopening completed child implementations without a concrete integration
  failure.
- Runtime-generated asm templates.

## Acceptance Criteria

- The review maps each umbrella completion criterion to a closed child, a
  current test, or a concrete follow-up gap.
- A representative custom-vector inline asm path is proven end to end, covering
  compile-time template text, `VRM*` constraints, `.insn.d` encoding, and object
  bytes through c4c's route.
- The review confirms scalar `.insn`, vector constraints, `.insn.d`, and
  consteval/template string behavior were not weakened or bypassed.
- If a gap is found, it is recorded as a new focused source idea under
  `ideas/open/` instead of silently expanding this review child.
- If no gap is found, this child can close and the parent umbrella can move to
  close review.

## Reviewer Reject Signals

- The review claims umbrella readiness without tying every completion criterion
  to a closed child, current test, or explicit follow-up gap.
- The composed route only works through a compiler-known EV mnemonic, intrinsic,
  external assembler proof, or hard-coded sample string.
- `VRM2` or `VRM4` allocation, overlap, or base-register substitution is not
  exercised in the same route that emits `.insn.d` object bytes.
- Consteval/template strings are accepted in frontend-only tests but bypass the
  inline asm metadata, operand binding, or RV64 object emission path.
- Existing tests are weakened, disabled, or reclassified to make the umbrella
  appear complete.
- Broad implementation or unrelated backend rewrites are mixed into the review
  instead of creating a focused follow-up child for a discovered gap.
- The parent umbrella is closed while a required child idea remains open or
  while this review has unresolved gaps.

## Closure Note

Closed 2026-06-24 after Steps 1-4 completed the integration review. The review
mapped every parent umbrella completion criterion to closed child evidence,
current tests, or explicit out-of-scope boundaries; proved the composed
literal/helper `.insn.d` route through `VRM*` constraints, base-register
substitution, EV 64-bit encoding, and RV64 object bytes; and recorded that no
concrete follow-up child gap is required before parent umbrella close review.

Close-time regression guard passed on the existing `test_before.log` /
`test_after.log` broader validation pair: 80 passed before and after, 0
failures, and no new failures. Remaining GNU named operands, `%c[...]`
modifiers, mask-specific constraints, broader GNU assembler compatibility, and
FPR `.insn` constraints remain outside this child unless opened separately.

## Parent

Parent idea: `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.
