Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement or Confirm the Agreement Boundary

# Current Packet

## Just Finished

Step 2: Implement or Confirm the Agreement Boundary completed.

`prepared_block_label_for_index(...)` now makes the block-index bridge boundary
explicit:

- invalid prepared labels remain `kInvalidBlockLabel`
- complete prepared/BIR agreement accepts the structured BIR `label_id`
- absent, short, invalid, or mismatched BIR rows keep the prepared fallback
- absent or short prepared control-flow keeps the existing BIR fallback

Focused helper rows in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` exercise the private
helper through `populate_call_plans(...)` by observing the frame-address
materialization label selected for a call argument.

## Suggested Next

Execute Step 3 from `plan.md`: add any remaining fail-closed proof rows around
the block-index label bridge, especially nearby rows the supervisor wants beyond
the Step 2 focused call-plan observations.

## Watchouts

- Do not reactivate completed idea 260 candidates.
- Keep scope limited to the block-index label bridge candidate.
- The Step 2 test observes the private helper through public call-plan
  population; no `call_plans.hpp` seam was added.
- Keep Step 3 test additions focused on the same private helper behavior. Do not
  rewrite expectations, output strings, helper statuses, or unsupported markers
  to claim progress.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Build was up to date and `backend_prepared_lookup_helper`
passed 1/1.

Proof log: `test_after.log`
