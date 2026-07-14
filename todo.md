# Current Packet

Status: Active
Source Idea Path: ideas/open/766_lir_ssa_indexed_gep_pointer_result_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove the contract and hand off to 764

## Just Finished

- 766 Step 1 is implemented and pending supervisor acceptance: the
  operand-preserving indexed-GEP route publishes a fresh result `LirValueId`
  only for an SSA pointer base and typed native index. The verifier retains
  the global-`LinkNameId` route and validates the SSA base as a
  current-function pointer definition; raw, invalid, foreign, non-pointer,
  and partial-index cases remain fail-closed.

## Suggested Next

- Complete 766 Step 2 by reviewable direct focused-proof confirmation of the
  SSA indexed-GEP contract, then hand the accepted result back to 764 for its
  separate downstream carrier-publication packet. Do not change the contract
  or perform 764 work in this packet.

## Watchouts

- Do not publish `LirIndirectBrOp.addr_value` or change `IndirBrStmt`; that is
  764 Step 1 after this blocker completes. Do not touch Raw-BIR/importer or
  734 Step 7.24.
- Do not weaken the verifier, accept raw/partial/text-derived authority, or
  introduce a synthetic cast, alloca/load, phi, or select bridge.
- The authoritative route is deliberately narrow: only an SSA pointer base
  plus a fully typed native index produces an indexed-GEP result ID. Legacy
  string-only indexed GEPs remain compatibility paths without a result ID.

## Proof

- Fresh `cmake --build --preset default` passed.
- `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed; output is preserved in `test_after.log`. The direct fixture covers a
  production local-pointer-plus-local-index GEP with valid base/index/result
  IDs, then rejects raw, invalid, foreign, non-pointer, and partial-index
  authority mutations. Existing nearby tests retain the global GEP path and
  its malformed cases.
