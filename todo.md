Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 closed out the Step 2 helper boundary for the removed indirect-register
byval local decision.

- No `aarch64_indirect_register_byval_argument` declaration, definition, or code
  caller remains.
- The remaining `prepared_indirect_byval_extent_bytes` boundary is intentionally
  retained as an emission-only shared predicate: `calls_preservation.cpp` uses it
  only to suppress prior-preserved source reuse for prepared indirect byval
  moves, while `calls_moves.cpp` uses the same prepared fact to choose the
  publication source. It does not restore a `CallInst::arg_abi` authority read.
- The remaining publication-helper blockers are still outside this closeout
  packet.

## Suggested Next

Supervisor route decision: either send the remaining publication-helper
blockers to a separate packet/plan review, or close this Step 3 slice as the
selected local-decision boundary cleanup.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  durable blockers remain.
- The publication blockers in `calls_dispatch_bridge.cpp` and
  `calls_argument_sources.cpp` remain separate durable candidates and were not
  touched by this packet.
- The `calls_dispatch_bridge.hpp` `CallInst`-shaped helper boundary remains a
  separate durable candidate.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.
- Do not restore `aarch64_stack_byval_argument_size_bytes`,
  `aarch64_indirect_byval_argument_size_bytes`, or
  `aarch64_indirect_register_byval_argument`.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed in `test_after.log`; 162/162 `^backend_` tests passed.
