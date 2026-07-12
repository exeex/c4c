# Current Packet

Status: Active
Source Idea Path: ideas/open/718_block_entry_publication_identity_completion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair genuine prepared attribution and prove the adapter

## Just Finished

- Reviewer report
  `review/idea718_step6_genuine_attribution_acceptance_review.md` rejected the
  uncommitted Step 6 slice: its query-local atomic minted a fresh synthetic
  lookup token, the tests manually copied that token into claims, and the
  production AArch64 path still used the fail-closed pointer overload.
- The rejected code and test changes were removed. Step 6 remains active.

## Suggested Next

- Implement a real producer-owned production claim collection: preparation must
  own or receive a stable nonzero attribution ID for the concrete proof claim,
  Route4 must carry that same ID without test-side copying, and production must
  classify the collection and call the authoritative MIR overload.

## Watchouts

- The later exception `x86::module::emit requires prepared core facts for every
  defined function` belongs to idea 721. Do not change x86 emission or fixture
  construction to bypass it while executing idea 718.
- Do not use query-local counters, coordinates, pointers, names, constants, or
  test-side claim assembly as attribution authority.
- Production currently calls the pointer overload in
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`; the next packet
  must explicitly own the production bridge before claiming acceptance.

## Proof

- Rollback proof: `git diff --check`.
- Rejected `test_after.log` removed; no canonical after-state proof is accepted
  for the rejected route.
