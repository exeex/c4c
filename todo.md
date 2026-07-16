Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.41
Current Step Title: Receive The 829 Fixed-Direct-Call Argument-1 Body-Parameter Row

# Current Packet

## Just Finished

Activated 734 after closed 829 handed off exactly one DirectScalar
body-parameter authority tuple for fixed direct-call argument 1. Accepted
Steps 1 through 7.40 remain historical work and must not be repeated.

## Suggested Next

Implement Step 7.41: add the typed Raw-BIR receiver/importer/verifier path for
closed 829's `FixedDirectCallArgument1` tuple and nearby transactional
positive/malformed coverage. Use the accepted Step 7.40 argument-0 receiver as
the closest pattern without broadening scope.

## Watchouts

- Do not edit the native LIR producer authority in 829.
- Do not receive argument 0 again or any other body-parameter row.
- Do not use text, names, signatures, rendered operands, diagnostics,
  `monostate`, or unclassified values as authority.
- Do not absorb memory/VA, aggregate/vector, module/type/global,
  instruction/terminator, inline-assembly, or generic call work.

## Proof

No Step 7.41 receiver proof has run yet.
