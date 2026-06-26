Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Same-Module Sret Evidence

# Current Packet

## Just Finished

Activated idea 387 as the next actionable RV64 object-route child after ideas
391 and 392 closed. The remaining open idea 354 is the umbrella that depends
on child closure, and idea 385 is an unrelated EV64 `.insn.d` encoding
contract.

## Suggested Next

Execute Step 1 from `plan.md`: refresh prepared/BIR and call-plan evidence for
`tests/c/external/gcc_torture/src/920908-1.c`, then record whether the
same-module sret call still fails at the `memory_return` admission gate.

## Watchouts

- Do not hard-code `920908-1.c`, callee `f`, stack offsets, or the current
  diagnostic.
- Do not delete the `memory_return` gate without proving correct sret address
  placement and post-call memory-result behavior.
- Keep ordinary scalar frame-slot-address call arguments outside this owner
  unless the evidence proves a shared abstraction is required.

## Proof

Activation-only lifecycle change. No build, runtime proof, or regression guard
was run by the plan owner during activation.
