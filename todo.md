# Current Packet

Status: Active
Source Idea Path: ideas/open/850_hir_signature_aggregate_ref_producer_order.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the production signature-fact seam and order

## Just Finished

- Lifecycle switch from 849: Step 1 is accepted at `109ea13f4`; Step 2 was
  interrupted with no implementation accepted because all production
  `lower_function` callers supplied the default null carrier.

## Suggested Next

- Execute Plan 850 Step 1: trace the production semantic/order callers before
  `lower_function` and establish the smallest direct HIR definition/ref fact
  seam for aggregate returns and explicit parameters.

## Watchouts

- A carrier that only validates then holds/discards input is not capability
  progress; the fact must exist in production before `lower_function`.
- Do not use parser/`TypeSpec`/owner/tag/text lookup, a `Node*` map,
  `qtype_from` attachment, or LIR.
- 848 remains parked; its Step 2a proof is `359a9b94b` / `frontend_hir_tests`.

## Proof

- No-code lifecycle switch. Step 1 must select and run a fresh build plus
  focused HIR proof if it changes semantic construction.
