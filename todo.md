# Current Packet

Status: Active
Source Idea Path: ideas/open/752_lir_local_object_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the selected local-object authority contract

## Just Finished

- Step 1 completed the current-function local object-pointer contract and
  repaired direct-local lvalue/rvalue producer plumbing. Focused coverage
  separately exercises alloca, direct local store, static-array indexed GEP,
  direct local load, VLA stack save, and backward-goto stack restore; each
  selected operation retains matching typed pointer/object/liveness authority
  after its display spelling is made misleading and verifier success is checked.

## Suggested Next

- Advance to Step 2's malformed-authority boundary checks, without widening
  into Raw-BIR/importer or Step 3 handoff work.

## Watchouts

- Stack restore is emitted only on a backward goto with a VLA lifetime route;
  ordinary VLA fixtures do not cover it. Keep authority selection keyed by
  LocalId/current-function facts, never local names, `%t`, or formatted
  operands. Keep Raw-BIR/importer, memory/va, aggregate/vector, PHI/CFG, and
  target-lowering outside this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log` passed (1/1). `test_after.log` contains the result.
