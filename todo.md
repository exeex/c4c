# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared call-boundary oracle

## Just Finished

Lifecycle activation created this execution state for Step 1. No implementation
packet has run yet.

## Suggested Next

Delegate Step 1 to inspect the prepared call-boundary oracle and classify the
semantic call argument/result source facts that BIR may own versus ABI,
prealloc, and codegen placement fields that must stay out of BIR.

## Watchouts

- Keep ABI register/stack placement, aggregate transport lanes, scratch,
  variadic state, preservation/clobber policy, destination homes, helper
  protocols, and final call lowering outside BIR.
- Treat prepared call plans and call-source helper queries as the oracle until
  BIR equivalence is proven one fact family at a time.
- Do not weaken tests, rewrite expectations, or add named-case shortcuts to
  claim progress.

## Proof

No proof run for lifecycle-only activation.
