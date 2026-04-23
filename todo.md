Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Grouped Bank And Span Authority To Prealloc
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan Step 2: published grouped callee-saved spans and grouped preserved-value
authority through prealloc call/frame contracts, including printer-visible
width/unit metadata for grouped preserved registers.

## Suggested Next

Thread grouped storage, spill/reload, and call-clobber semantics through the
remaining prealloc publication surfaces without forcing consumers to infer
grouped homes on their own.

## Watchouts

- Keep the route target-independent.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(prepare_frame_stack_call_contract|prepared_printer)$'`
