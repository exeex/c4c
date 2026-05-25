Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 removed the selected retained ABI-shape decision:
`aarch64_indirect_register_byval_argument`.

- `make_prior_preserved_call_argument_source` now suppresses prior-preserved
  source reuse for large indirect byval moves through the prepared-only fact
  `prepared_indirect_byval_extent_bytes(context, move, argument, *source_home)`.
- `prepared_indirect_byval_extent_bytes` is exposed from `calls_moves.cpp`
  through `calls.hpp` without reintroducing a `CallInst::arg_abi` authority
  read.
- The `aarch64_indirect_register_byval_argument` declaration and definition
  were deleted.

## Suggested Next

Step 3 boundary closeout: confirm the selected helper removal is the complete
local decision cleanup for this route, then choose whether the remaining
publication-helper blockers need a separate plan step or durable follow-up
idea instead of widening this packet.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  durable blockers remain.
- The publication blockers in `calls_dispatch_bridge.cpp` and
  `calls_argument_sources.cpp` remain separate durable candidates.
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
