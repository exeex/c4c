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

## Active Baseline-Blocker Intake

795 is activated from 810's rejected post-`1f1a1fb38` full baseline. The
selected surface is a variable RHS function-body parameter used as the index
of a direct pointer-compound-assignment GEP (`pr21173.c` diagnostic:
`LirGepOp.indices.value: authoritative GEP index requires integer or SSA
authority`). Trace and repair only the native parameter body-use/index
handoff, including exact classification if its ABI form requires it. This does
not reopen 810's accepted native-immediate producer repair, reconstruct
identity from text, or weaken the GEP verifier. On accepted focused proof,
return 810 at unchanged Step 3.

## Reviewer Reject Signals

- Reject reliance on closed 742 declaration publication as body-use authority.
- Reject broad ABI conversion, receiver edits, expectation weakening, or
  rendered parameter names/types as semantic authority.

## Resumption / Completion Record

- Accepted progress: Step 1 trace is recorded in `7807812ee`; Step 2 native
  body-parameter authority publication is accepted in `281737387`.
- Accepted proof: fresh `cmake --build --preset default`; backend CTest 5/5
  with the accepted non-decreasing regression guard; focused
  `backend_lir_selected_pointer_authority`; and successful
  `./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/pr21173.c`.
- Disposition: the bounded parameter-index handoff is complete. Its required
  return-control action is this lifecycle switch: resume the already-open 810
  blocker unchanged at Step 3 for a fresh comparable full baseline. This does
  not establish 3037/3037 clearance or return control to 801.
