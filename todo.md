# Current Packet

Status: Active
Source Idea Path: ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the bounded direct carrier/API

## Just Finished

- 850 Step 1 concluded no-change: production callers have only `Node`/template
  inputs, so no pre-carrier aggregate fact can reach `lower_function` without
  the carrier/API owned by 849. No code or tests changed.

## Suggested Next

- Execute Plan 849 Step 2 from the accepted `109ea13f4` seam: implement the
  direct carrier and make production callers deliver only existing
  module-issued facts (or no fact) through `lower_function`.

## Watchouts

- A carrier that only validates then holds/discards input is not capability
  progress; production callers must use the carrier to deliver an existing
  direct fact where their construction context provides one.
- Do not use parser/`TypeSpec`/owner/tag/text lookup, a `Node*` map,
  `qtype_from` attachment, or LIR.
- 848 remains parked; its Step 2a proof is `359a9b94b` / `frontend_hir_tests`.

## Proof

- 850 no-change probe already passed a fresh `cmake --build --preset default`
  and `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`.
  Step 2 is code-bearing and requires a fresh build plus focused HIR proof.
