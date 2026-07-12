# Current Packet

Status: Active
Source Idea Path: ideas/open/718_block_entry_publication_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair genuine prepared attribution and prove the adapter

## Just Finished

- Plan-owner route review rejected the current Step 6 adapter because it
  synthesizes `attribution_id` as
  `publication_bundle_instruction_index + 1`; instruction position is not the
  independent proof-claim authority required by idea 718 and the accepted idea
  720 contract.
- The later x86 missing-prepared-core emission abort was split out to
  `ideas/open/721_x86_defined_function_prepared_core_completion.md` and is not
  part of idea 718 acceptance.

## Suggested Next

- In Plan Step 6, publish the genuine proof-claim attribution identity from the
  prepared producer, copy it unchanged through the MIR adapter, and add the
  direct positive/fail-closed adapter matrix before rerunning production-facing
  identity proof.

## Watchouts

- Do not accept coordinate-, pointer-, display-name-, or constant-derived
  attribution as a substitute for the preparation-supplied claim identity.
- `backend_prepared_lookup_helper` passing does not prove the prepared-to-Route4
  adapter; require a direct adapter matrix and independently green
  production-facing identity evidence.
- The later exception `x86::module::emit requires prepared core facts for every
  defined function` belongs to idea 721. Do not change x86 emission or fixture
  construction to bypass it while executing idea 718.
- Preserve the conservative pointer compatibility contract; availability must
  continue to require attributed prepared proof plus Route4 classification.

## Proof

- Reviewer route audit:
  `review/idea718_step6_identity_repair_route_review.md`.
- Existing proof is not acceptance-ready: it exercises the Route4 classifier
  and reaches a later abort, but does not prove genuine prepared attribution or
  the adapter's fail-closed matrix.
