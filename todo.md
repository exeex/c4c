# Current Packet

Status: Active
Source Idea Path: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind focused probes to native operand/result contracts

## Just Finished

- Completed 783 Plan Step 3: added fail-closed frontend-LIR structural probes
  for the AArch64 GP native `gr_top` load to indexed `reg_addr` GEP boundary,
  AArch64 FP ptrmask-result to aligned-stack GEP boundary, and AMD64 native
  register-GEP/stack-load to memcpy boundaries plus value-only PHI transport.

## Suggested Next

- Compare the three accepted Step 3 contracts and select the common minimal
  native operand/result publication for 783 Plan Step 4.

## Watchouts

- Do not add PHI verification, predecessor/edge identity, CFG semantics, 782
  helper fields, HFA ptrmask work, Raw-BIR/importer, backend, target lowering,
  MIR, emission, generic migration, or text-derived identity.
- The AMD64 probe observes `LirPhiIncoming.value` only as value transport; it
  makes no predecessor, edge, or PHI-verifier claim.

## Proof

- Step 3 focused proof passed 1/1: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  Root regression logs were not written; they remain supervisor-owned.
