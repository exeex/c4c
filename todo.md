Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Plan Step 3: added a grouped x86 consumer-surface proof in
`backend_prepare_frame_stack_call_contract_test` that checks `x86::consume_plans`
reads grouped storage, preserved-value, callee-saved, and clobber authority
directly from prepared plans.

## Suggested Next

Review whether another Step 3 consumer beyond the prepared dump and x86
consumer surface still lacks a direct grouped authority contract.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'` passed (`test_after.log`)
