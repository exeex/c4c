Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Block-Index Label Bridge Surface

# Current Packet

## Just Finished

Step 1: Locate the Block-Index Label Bridge Surface completed.

Selected helper surface:

- `src/backend/prealloc/call_plans.cpp` owns the private
  `prepared_block_label_for_index(const PreparedControlFlowFunction*, const
  bir::Function&, std::size_t)` helper.
- Direct call sites are both in `call_plans.cpp`: byval/source selection uses
  `call_plan.block_index`, and `populate_call_plans(...)` uses the loop
  `block_index` while building each `PreparedCallPlan`.
- Current contract is index-first and permissive: if prepared control-flow is
  present and the index is in range, it returns
  `control_flow->blocks[block_index].block_label`; otherwise it returns
  `function.blocks[block_index].label_id`; if both are out of range it returns
  `kInvalidBlockLabel`.
- Current fallback/invalid-label behavior: invalid prepared labels are returned
  as-is when the prepared row exists; absent/short prepared control-flow falls
  back to the BIR block `label_id`; an out-of-range BIR index returns
  `kInvalidBlockLabel`. There is no current prepared/BIR agreement check for
  stale, mismatched, or invalid structured labels at this helper.
- Nearby reusable agreement/fail-closed patterns live in
  `src/backend/prealloc/lookup_agreement.cpp` and
  `src/backend/prealloc/control_flow.hpp`.
- Focused helper-test surface is
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, especially the
  existing module lookup-reader agreement rows and branch-target identity rows.
  The exact helper is private today, so Step 2 tests should exercise reachable
  call-plan behavior through `populate_call_plans(...)` or add the smallest
  local/test-visible seam if direct helper assertions are necessary.

## Suggested Next

Execute Step 2 from `plan.md`: implement or confirm the agreement boundary in
`src/backend/prealloc/call_plans.cpp`, with focused proof rows in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. Include
`src/backend/prealloc/call_plans.hpp` only if a public/test-visible helper seam
is required.

## Watchouts

- Do not reactivate completed idea 260 candidates.
- Keep scope limited to the block-index label bridge candidate.
- Preserve prepared fallback and `kInvalidBlockLabel` behavior when
  prepared/BIR agreement is incomplete.
- Do not rewrite expectations, output strings, helper statuses, or unsupported
  markers to claim progress.
- The helper currently accepts prepared row identity by vector index without
  checking that the prepared row's label agrees with the BIR block at the same
  index. Step 2 should fail closed for stale/mismatched/invalid structured BIR
  facts while preserving prepared fallback when BIR agreement is absent or
  incomplete.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Build was up to date and `backend_prepared_lookup_helper`
passed 1/1.

Proof log: `test_after.log`
