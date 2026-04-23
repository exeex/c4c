Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Plan Step 3: added a focused grouped storage/call consumer proof in
`backend_prepare_frame_stack_call_contract_test` and repaired storage-plan
publication so ABI-home parameter registers keep scalar bank/span metadata even
when regalloc also tracks a separate grouped assignment for the same value.

## Suggested Next

Review whether another Step 3 consumer beyond frame/stack/call and prepared
dump still reads grouped publication through an ABI-home seam that lacks direct
proof.

## Watchouts

- Keep the route target-independent.
- Register-backed storage publication should only carry grouped bank/span
  metadata when the published home is the assigned grouped register itself.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'` passed (`test_after.log`)
