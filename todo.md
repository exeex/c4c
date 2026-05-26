Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Private Calls Helpers Into `calls.cpp`

# Current Packet

## Just Finished

Step 2 - Fold Private Calls Helpers Into `calls.cpp` completed a narrow
mechanical fold-back packet for the variadic va_start overflow-area helper:

- Moved `va_start_overflow_area_stack_offset` and its private helper chain
  (`entry_param_uses_incoming_stack`, `named_incoming_stack_bytes`,
  `function_has_call`, `fixed_frame_adjustment_bytes`, and the incoming-stack
  alignment helpers) from `calls_common.cpp` into the anonymous namespace in
  `calls.cpp`.
- Removed the `va_start_overflow_area_stack_offset` declaration from
  `calls.hpp`.
- Left `outgoing_stack_argument_bytes` and
  `outgoing_stack_argument_base_register` exported from `calls_common.cpp` for
  `calls_moves.cpp`.
- Kept behavior, variadic handling, call planning, diagnostics, and test
  expectations unchanged.

## Suggested Next

Next fold-back packet: continue narrowing the remaining `calls_common.cpp`
surface only for helpers with a clear single-file owner, while preserving the
externally consumed call-move and dispatch bridge APIs.

## Watchouts

`outgoing_stack_argument_bytes` and `outgoing_stack_argument_base_register`
remain shared with `calls_moves.cpp`. `make_selected_call_argument_source` and
`find_move_bundle` remain externally visible for the bridge; avoid folding
dispatch-facing APIs into a calls-only packet.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
