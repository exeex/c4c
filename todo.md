# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Disposition the ALU return-chain authority gap

## Just Finished

- None since the Step 2 route reset.

## Suggested Next

- Execute Plan Step 2.1 only: determine whether an existing common typed query
  owns the ALU return-chain relation before changing another consumer family.

## Watchouts

- Do not preserve `find_prepared_return_chain_facts` by renaming its move,
  successor-home, scalar-producer, or operand reconstruction.
- If no traversal-attached common query expresses the return-chain relation,
  stop and report the query-contract gap; producer redesign is outside idea 709.

## Proof

- Not run; lifecycle-only route reset.
