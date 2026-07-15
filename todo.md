# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair anonymous layout / structured-call compatibility

## Just Finished

- Step 1 trace/selection is accepted in `827dae5bd3`; the bounded native
  anonymous-layout contract and its 754 handoff boundary were identified.
- 802 Step 1's isolated switch verifier/test working-tree changes are
  unaccepted and parked for resumption. Its exact focused command is
  `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`.
- The preserved unaccepted 801 Step 2 repair fails that command before the
  switch check at `LirCallOp.arg_type_refs`: `argument 0 mirror does not match
  call text; shadow 'i32', call argument type 'rendered arguments are not'`.

## Suggested Next

- Step 2: repair the native argument-mirror verifier compatibility defect that
  prevents reaching the switch check. Keep native field facts checked and do
  not weaken mirror or signature contracts. Once the focused test reaches the
  switch check, return to 802 Step 1 with its preserved hunk/tests.

## Watchouts

- Do not parse `LirTypeRef` display text, use rendered diagnostics as argument
  type authority, weaken `LirCallOp` mirror/signature checks, or add
  extractvalue field/index/result validation, Raw-BIR, or generic aggregate
  work. 802's work remains unaccepted and must not be claimed as 801 progress.

## Proof

- Immediate prerequisite proof: `ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure` reaches the switch
  selector check. After 802 resumes, retain the Step 2 ladder: fresh `cmake
  --build --preset default`; focused call/frontend-HIR/backend tests; then the
  supervisor-owned full baseline and explicit acceptance before Step 3.
