# Current Packet

Status: Active
Source Idea Path: ideas/open/766_lir_ssa_indexed_gep_pointer_result_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define and publish SSA-based indexed-GEP pointer-result authority

## Just Finished

- Switched from 764 Step 1 after its preserved investigation established that
  `emit_indexed_gep` is the upstream raw-result/string-only owner. The accepted
  765 RHS identity handoff is available; no code or test edit was accepted for
  the rejected pointer-cast bridge.

## Suggested Next

- Inspect the authoritative-GEP verifier contract and direct
  `emit_indexed_gep` producer, then implement only the verifier-valid
  SSA-based pointer-result authority required by Step 1.

## Watchouts

- Do not publish `LirIndirectBrOp.addr_value` or change `IndirBrStmt`; that is
  764 Step 1 after this blocker completes. Do not touch Raw-BIR/importer or
  734 Step 7.24.
- Do not weaken the verifier, accept raw/partial/text-derived authority, or
  introduce a synthetic cast, alloca/load, phi, or select bridge.

## Proof

- Incoming evidence: `cmake --build --preset default` passed for the rejected
  stmt-only pointer-cast route; `ctest --test-dir build -j --output-on-failure
  -R '^frontend_lir_call_type_ref$'` then failed because authoritative casts
  require integer endpoints. This is a rejected contract, not a proof target.
- Before this idea's handoff, run a fresh build plus focused direct
  SSA-indexed-GEP positive and malformed-authority coverage selected from the
  nearby verifier/producer test surface.
