# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify row-specific index or mask facts

## Just Finished

- Completed resumed 754 Plan Step 2. `LirExtractValueOp` now has the minimum
  opt-in `requires_native_result_authority` marker. Only the closed-798 unary
  complex aggregate carrier path sets it: when `agg` already has the checked
  `LirOperand::ssa`/`LirValueId`, lowering allocates its result with
  `fresh_value(ctx)` and forwards the exact aggregate operand unchanged.
- The verifier requires valid SSA result and aggregate IDs for that opt-in
  marker, includes native extract results in current-function foreign-owner
  checks, and retains the 798 call-result/aggregate-type/display-mirror gate.
  Printer rendering remains the existing compatibility rendering. Unselected
  extractvalue producers and all other aggregate/vector rows remain legacy
  compatibility/fail-closed paths. No index or element-result type semantics
  were added.

## Suggested Next

- Step 3 only: add `LirExtractValueOp` aggregate field/index and result-type
  coherence validation plus its focused proof; do not widen to other rows.

## Watchouts

- Do not reopen 798, recover IDs from display text, widen to other
  aggregate/vector rows, or add aggregate index semantics before Step 3.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_|frontend_hir_tests$)' > test_after.log`
  passed: fresh build succeeded and the focused frontend/backend subset passed
  6/6. `test_after.log` is the Step 2 proof log.
