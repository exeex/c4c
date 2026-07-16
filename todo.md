# Current Packet

Status: Active
Source Idea Path: ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the direct-scalar truthiness-LHS authority seam

## Just Finished

- 832 closed capability-complete for its bounded HIR aggregate-owner crash:
  `b556c6c9f` changed the matching HIR guard from pre-repair 0/1 segfault to
  accepted 1/1 pass, recorded by `b31cfa6ec`. This does not clear the baseline.

## Suggested Next

- Trace the selected direct-scalar truthiness LHS from construction into
  `verify_truthiness_lhs_parameter_authority` and identify the exact missing
  native authority relation. Keep this route separate from HIR/832 and 830.

## Watchouts

- Do not use rendered text, signatures, diagnostics, default classification,
  named-case exceptions, filters, unsupported markers, or weaker contracts.
  Return only to 831 Step 2 after accepted focused proof.

## Proof

- Evidence predecessor: 831's exact subset retains 13 GCC torture failures at
  the unchanged missing `LirCmpOp.truthiness_lhs_parameter_authority` verifier
  relation. Step 1 narrows this native producer/verifier seam before code.
