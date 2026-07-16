# Current Packet

Status: Active
Source Idea Path: ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the signature semantic carrier seam

## Just Finished

- Lifecycle switch: 848 Steps 1 and 2a remain accepted; 848 Step 2b is parked
  pending this distinct upstream direct semantic-construction carrier.

## Suggested Next

- Trace the return and parameter semantic construction routes and identify the
  smallest direct definition-backed carrier/API that reaches both lowering
  call sites without type-derived recovery.

## Watchouts

- Do not use parser, `TypeSpec`, `record_def`, owner/structured-owner, tag,
  text, parser-pointer, `Node*` map, or reconstructed lookup as canonical
  identity.
- Do not edit `qtype_from`, attach occurrence refs, or change LIR; those are
  outside this blocker and 848 resumes Step 2b after acceptance.

## Proof

- No blocker implementation or proof is accepted yet. Parent evidence retained
  in 848: commit `359a9b94b`; focused `frontend_hir_tests` command passed and
  is recorded in `test_after.log`.
