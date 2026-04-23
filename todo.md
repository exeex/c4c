Status: Active
Source Idea Path: ideas/open/89_grouped_register_bank_and_storage_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Grouped Storage, Spill, Reload, And Clobber Semantics
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Plan Step 3: preserved grouped register-span authority on spill eviction and
published that bank/span plus stack-slot metadata through `spill_reload_ops`
and prepared-printer output for grouped LMUL-style vector spans.

## Suggested Next

Review whether any Step 3 storage/call consumers beyond the prepared dump need
focused proof now that grouped spill/reload authority is present end-to-end.

## Watchouts

- Keep the route target-independent.
- Storage homes that still point at ABI parameter registers must not inherit
  grouped occupied-unit metadata from separate regalloc assignments.
- Do not push grouped legality reasoning into emitters or backend consumers.
- Reject testcase-shaped grouped-register special cases.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_prepare_frame_stack_call_contract)$'` (`test_after.log`)
