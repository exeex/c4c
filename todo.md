Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 completed the helper-boundary closeout for the selected stack byval size
path.

Completed work:

- Verified there is no surviving source declaration, definition, or call site
  for `aarch64_stack_byval_argument_size_bytes`.
- Confirmed the remaining matches are lifecycle text in `plan.md` and
  `todo.md`, not an active helper boundary.
- Re-ran the focused build and AArch64 backend boundary tests with no include
  or build fallout.

## Suggested Next

Step 4 broader backend checkpoint for the selected stack byval path.

## Watchouts

- The selected path now prefers the source home size to preserve the aggregate
  copy extent and falls back only to prepared destination stack size. It does
  not rederive byval shape or size from retained `CallInst::arg_abi`.
- This packet intentionally did not widen into
  `aarch64_indirect_byval_argument_size_bytes`,
  `aarch64_indirect_register_byval_argument`, or local address publication
  helpers.

## Proof

Step 3 focused proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner)$') > test_after.log 2>&1`

Result: passed, `2/2` focused tests in `test_after.log`.
