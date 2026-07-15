# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish structured result and operand authority

## Just Finished

- Lifecycle resume: 754 Step 1 was accepted in `d8e5ed3a8`; blocker 798 is
  capability-complete with the accepted handoff in `2c231e342`. No 754 Step 2
  implementation is accepted yet.

## Suggested Next

- Step 2 only: publish the minimum opt-in `LirExtractValueOp` structured
  result and aggregate-use authority, consuming the exact checked operand
  carried by 798. Leave index semantics for Step 3.

## Watchouts

- Do not repeat Step 1, reopen 798, recover IDs from display text, widen to
  other aggregate/vector rows, or edit Raw BIR.

## Proof

- Before accepting Step 2, run a fresh build and the delegated same-feature
  proof. 798's accepted 6/6 before/after subset is prerequisite evidence, not
  754 Step 2 proof.
