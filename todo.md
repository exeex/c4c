# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.32
Current Step Title: Resume direct external double call-result authority

## Just Finished

- Idea 746 closed after commit `0d3781e70` proved the corrected prerequisite:
  a block-scope fixed-void extern declaration becomes a normal bodyless extern
  HIR `Function`; its LIR declaration/direct call share `LinkNameId` and
  structured signature, and its scalar result owns the `LirValueId` used by
  `FAdd`.

## Suggested Next

- Resume Plan Step 7.32 at the interrupted external-double packet. Finish its
  focused producer/verifier/matrix contract using the normal resolved extern
  `Function` route, including reachable malformed declaration, signature,
  result-ownership, uniqueness, and type-conflict obligations.

## Watchouts

- Do not redo Steps 1-6 or earlier Step 7 packets. Do not create a separate
  extern-function list, recover facts from display text, or restore the stale
  absent-`target_fn` route. Step 7.32 is unblocked, not yet complete.

## Proof

- Accepted prerequisite proof: fresh default build; 8/8 related
  parser/HIR/LIR/BIR tests; matching `frontend_lir_call_type_ref` guard 1/1
  before and 1/1 after with no new failures.
