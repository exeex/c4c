Status: Active
Source Idea Path: ideas/open/853_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace The Next Body-Parameter Candidate

# Current Packet

## Goal

Trace the next valid function-body parameter-use row after accepted 734 Step
7.41 and decide whether the route can publish one native structured authority
tuple directly or needs a narrower prerequisite.

## Scope

- Preserve accepted 734 Steps 1 through 7.41, most recently `380ee782f`.
- Do not edit Raw BIR, importer, builder, or receiver/verifier code.
- Do not repeat accepted DirectPointer or DirectScalar GEP, binary-LHS,
  binary-RHS, ReturnValue, switch-selector, truthiness-comparison-LHS,
  fixed-direct-call argument-0, or fixed-direct-call argument-1 rows.
- Do not infer identity, type, owner, ABI, role, or consumer coherence from
  text, names, rendered operands, diagnostics, signatures, compatibility
  mirrors, or testcase shape.

## Work Items

- Inventory existing LIR body-parameter authority carriers and accepted
  producer/receiver rows.
- Trace remaining produced function-body parameter uses and identify the first
  bounded semantic relation with native current-function value, owner,
  parameter index, type, ABI, role, and consumer relation evidence.
- If the candidate lacks structured producer authority, record the exact
  narrower prerequisite instead of implementing.
- Update this packet with the selected Step 2 producer/verifier route or the
  prerequisite lifecycle action.

## Proof

Read-only trace packet; no build required unless the trace runs local probes.
