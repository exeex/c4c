# LIR Function-Body Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/closed/793_lir_to_new_bir_remaining_coverage_umbrella.md`
Evidence predecessor: `ideas/closed/742_lir_function_parameter_authority_publication.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish structured function-body parameter identity for one bounded parameter
surface and classify ABI-expanded forms without treating declaration facts as
body-use authority.

## In Scope

- Establish native body-use identity and ownership for one logical parameter
  surface, including exact classification of any ABI-expanded form it needs.
- Verify malformed, foreign, type-incoherent, or display-derived facts reject;
  publish an exact future one-row receiver handoff only if proved.

## Out Of Scope

- Raw-BIR receipt, all ABI/byval/HFA/vector/variadic forms, module signatures,
  aggregate/vector value work, or text-derived identity.

## Acceptance Criteria

- One body-parameter surface has a checked structured contract and focused
  same-feature positive/negative proof; all other forms remain classified or
  fail closed.

## Reviewer Reject Signals

- Reject reliance on closed 742 declaration publication as body-use authority.
- Reject broad ABI conversion, receiver edits, expectation weakening, or
  rendered parameter names/types as semantic authority.
