Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Scalar-Control-Flow Evidence

# Current Packet

## Just Finished

Step 1 refreshed scalar-control-flow producer evidence for
`ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
Focused coverage remains present in `backend_prepare_phi_materialize`,
`backend_prepare_block_only_control_flow`, `backend_prepare_structured_context`,
`backend_lir_to_bir_notes`, and `backend_riscv_object_emission`.

Representative refresh:
- `src/20000314-3.c` / `attr_eq`: `--dump-bir`, `--dump-prepared-bir`,
  and `--dump-mir` all succeed; RV64 object route still fails at
  `unsupported_terminator_fragment`, a downstream RV64 object-lowering boundary,
  not the original `scalar-control-flow semantic family` producer boundary.
- `src/20030408-1.c`: direct RV64 object runner now passes.
- `src/20000622-1.c`: direct RV64 object runner now passes.
- `src/20000819-1.c`: direct RV64 object runner now passes.

No refreshed representative still fails at the original scalar-control-flow BIR
semantic producer boundary, so Step 1 does not expose a real Step 2 producer
repair.

## Suggested Next

Supervisor should hand this to the plan owner for a close decision or Step 3
closure handoff, with the only residual noted boundary being downstream RV64
object terminator lowering for `src/20000314-3.c` / `attr_eq`.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route this packet into function-signature, scalar-binop, local-memory,
  RV64 ABI, or object-emission work.
- The `attr_eq` residual is downstream RV64 object lowering
  (`unsupported_terminator_fragment`), not a scalar-control-flow BIR producer
  reason to edit `src/backend/bir/lir_to_bir/module.cpp`.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.

Result: build completed with no work to do; backend subset passed
`346/346` tests. Proof log: `test_after.log`.
