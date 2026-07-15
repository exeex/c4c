# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_operand_provenance_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove and publish the 754 handoff

## Just Finished

- Completed Plan Step 3. Focused HIR-to-LIR coverage lowers direct
  `__complex__ float` calls consumed by both `__real__` and `__imag__`; each
  observed `LirExtractValueOp.agg` carries the same native `LirValueId` and
  display mirror as its aggregate `LirCallOp.result`.
- Focused verifier coverage starts from that structured aggregate call/extract
  shape and rejects an SSA spelling with no ID, unknown/foreign authority,
  stale/misleading display, and return-type-incoherent `agg_type`. The accepted
  754-consumable contract is: an opt-in direct composite call defines
  `LirOperand::ssa(display, LirValueId)`; unary real/imag extraction forwards
  that exact operand as `agg`; the ID must resolve to the same current-function
  call result with equal structured return/aggregate `LirTypeRef`, and display
  is only a checked mirror. Raw legacy operands remain compatibility paths;
  no text recovery, schema/result/index, Raw-BIR, or receiver work was added.

## Suggested Next

- Lifecycle handoff only: have the plan owner accept this completed blocker,
  reactivate 754 at its unchanged Step 2, and consume the published aggregate
  operand contract there.

## Watchouts

- 798 does not publish `LirExtractValueOp` row result/index semantics. 754
  owns its unchanged Step 2; it may consume only this checked aggregate operand
  carrier and must not recover authority from text.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_|frontend_hir_tests$)' > test_after.log`
  passed: build succeeded and the selected frontend/backend subset passed 6/6.
  `test_after.log` is the proof log.
