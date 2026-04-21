# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Make X86 Rejection Diagnostics Plain And Actionable
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2.2 completed for the x86 route-debug surface and nearest regression
coverage. `src/backend/mir/x86/codegen/route_debug.cpp` now reports one final
plain-language rejection plus a next-inspection pointer when no top-level x86
lane matches, and classifies missing prepared-contract detail when the lane
already throws that distinction. `tests/backend/backend_x86_route_debug_test.cpp`
now locks ordinary route miss versus missing prepared handoff behavior, and the
separate `00204.c` `--dump-mir`/`--trace-mir` observations confirm `match`
ends with a meaningful final x86 rejection instead of the old generic top-level
lane miss.

## Suggested Next

Pick the next idea-67 packet that expands actionable x86 rejection detail past
the current ordinary-miss fallback, most likely by teaching one more bounded
lane to surface a contract-specific or shape-specific final detail for
multi-block functions like `match`.

## Watchouts

- The current classifier can distinguish missing prepared-contract detail only
  when a lane already throws that contract-specific exception; plain lane
  `nullopt` fallthroughs still resolve to the generic ordinary-miss wording.
- `00204.c` `match` now points developers at
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp` as the next top-level
  surface, but it is still an ordinary route miss rather than a more specific
  prepared-contract rejection.
- The supervisor-selected backend subset remains red on
  `backend_prepare_liveness` before this slice, and that failure stops the
  exact delegated proof chain before the trailing `00204.c` commands unless
  those observations are rerun separately.

## Proof

Ran the delegated proof command exactly as assigned:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_" && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c`
with output preserved in `test_after.log`. The exact chain still stops at the
pre-existing `backend_prepare_liveness` failure, so the trailing `00204.c`
commands were rerun separately to confirm the new x86 rejection output for
`match`.
