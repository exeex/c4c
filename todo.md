# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Audit the boundary and run integration proof

## Just Finished

- Completed Step 6: audited all remaining prealloc route/agreement vocabulary and
  found no in-scope executable or public-payload violation requiring a code fix.
- AST-backed inspection confirmed migrated named-fact selection consumes stable
  prepared identities and explicit fail-closed boundary statuses, without
  consulting route status or agreement records; the public-header umbrella guard
  reports no route-numbered record or index authority.

## Suggested Next

- Ask the plan owner to decide whether idea 705 is complete and should close.

## Watchouts

- Private observational compatibility: `lookup_agreement.*` and the prepared
  printer's body-text agreement remain lookup/printing checks, not inputs to the
  migrated named-fact selectors.
- Deferred scope: call-preservation and local-materialization route enums,
  call-argument BIR source routing, and select-chain agreement helpers belong to
  downstream call/materialization/select-chain work outside idea 705.
- Debug/proof-only route vocabulary remains in prepared contract-verifier status
  names. Named BIR boundary negatives remain explicit (`Missing`, `Incomplete`,
  `Ambiguous`, `Mismatched`, `Unsupported`) and fail closed.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log`.
- Result: 329/329 backend tests passed, including the prepared-fact boundary
  umbrella guard and explicit negative/parallel-edge probes; canonical proof log
  is `test_after.log`.
