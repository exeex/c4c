Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan Step 3: published grouped storage width/unit authority through
`storage_plans` and extended call-clobber publication so grouped caller-saved
vector spans appear in prealloc call contracts and prepared-printer output.

## Suggested Next

Thread grouped spill/reload publication through the remaining prealloc
contracts so grouped evictions and reload points carry bank/span authority
without downstream inference.

## Watchouts

- Keep the route target-independent.
- Storage homes that still point at ABI parameter registers must not inherit
  grouped occupied-unit metadata from separate regalloc assignments.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_frame_stack_call_contract|prepared_printer)$'` (`test_after.log`)
