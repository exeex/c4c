# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the blocked failure-family baseline

## Just Finished

- Switched from blocked idea 734 to the structured LIR operand/terminator
  identity decomposition initiative; no implementation progress is claimed.

## Suggested Next

- Reproduce and record the exact first identity boundary for the four existing
  focused producer cases, classifying only the structured and textual facts
  available at each failure.

## Watchouts

- Step 1 is evidence-only: do not edit LIR schema, producers, verifier, new BIR,
  or importer code before the authority matrix and probe bindings exist.
- Do not parse or match operand text, `value_str`, `type_str`, symbol spelling,
  printer output, or testcase identity.

## Proof

- Pre-switch backend proof at HEAD `fef9378c1` is fresh 4/4.
- Accepted full-suite baseline is 3033/3033; the route switch adds no code.
