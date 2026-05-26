Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Private Calls Helpers Into `calls.cpp`

# Current Packet

## Just Finished

Step 2 - Fold Private Calls Helpers Into `calls.cpp` completed the
calls-moves-owned common-helper relocation packet:

- Moved calls-moves-only operand/view/F128 helper definitions from
  `calls_common.cpp` into the anonymous namespace in `calls_moves.cpp`.
- Removed their declarations from `calls.hpp`, leaving them namespace-local to
  `calls_moves.cpp`.
- Kept `outgoing_stack_argument_bytes`,
  `outgoing_stack_argument_base_register`, and
  `scalar_integer_register_view_from_size` exported because they still have
  non-`calls_moves.cpp` callers.
- Kept behavior, diagnostics, ABI semantics, and test expectations unchanged.

## Suggested Next

Next fold-back packet: inspect the remaining `calls_common.cpp` surface and
either fold another clearly single-owner helper group or hand back for
supervisor/plan-owner review if no mechanical single-owner group remains.

## Watchouts

`outgoing_stack_argument_bytes` is still used by `calls.cpp` and
`calls_moves.cpp`; `outgoing_stack_argument_base_register` is still used by
`calls_moves.cpp`; `scalar_integer_register_view_from_size` is still used by
`calls_moves.cpp` and `calls_dispatch_bridge.cpp`. Keep dispatch-facing APIs
such as `make_selected_call_argument_source` and `find_move_bundle` exported.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
