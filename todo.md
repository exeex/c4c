# Current Packet

Status: Active
Source Idea Path: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove the logical RHS result authority contract

## Just Finished

- Plan Step 1 complete: the logical integer RHS non-`i1` `zext` now allocates
  its `LirCastOp.result` through `fresh_value(ctx)` and retains that native
  `LirOperand` at the producer; the raw PHI result/incoming boundary remains
  unchanged.

## Suggested Next

- Dispatch Plan Step 2: update the obsolete no-authority assertion with
  focused positive and malformed authority coverage for this logical RHS
  `LirCastOp.result` only.

## Watchouts

- The fixed proof command builds successfully but the selected test still has
  its obsolete pre-Step-2 assertion that the RHS result lacks authority. Step
  2 owns updating that assertion and adding malformed authority coverage; PHI
  result/incoming, the final logical consumer, generic expression APIs, and
  other producer families remain excluded.

## Proof

- Step 1 attempted: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^frontend_lir_call_type_ref$'`. Build passed; the
  selected test reached its obsolete pre-Step-2 assertion. The active runbook
  directs no root-log writes for this focused proof.
