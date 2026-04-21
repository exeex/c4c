# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3 now carries the large-case focus contract one seam deeper: the same
`--mir-focus-block` selector that already narrowed MIR summary/trace can now
also narrow `--dump-bir` and `--dump-prepared-bir` inside a focused function.
The backend dump path now trims semantic BIR blocks, prepared control-flow and
addressing records, and block-indexed prepared move bundles to the requested
block while still preserving the stable focus header and matched-count cues.
`tests/backend/CMakeLists.txt` adds honest `00204.c` CLI coverage for matching
and missing block focus on semantic and prepared dumps so the `stdarg` handoff
lane can be followed at one block across semantic BIR, prepared BIR, MIR
summary, and MIR trace without local instrumentation.

## Suggested Next

If idea 67 still needs another packet, keep it on a real remaining search-space
gap such as semantic/prepared value focus or a clearer missing-block contract
in dump mode. Do not spend more packets rephrasing already-readable
function/block focus output unless a large-case proof shows a concrete
inspection failure.

## Watchouts

- `--mir-focus-block` still requires `--mir-focus-function`; do not turn it
  into an unscoped module-level block matcher.
- Focused prepared output now filters block-owned records, but function-level
  notes and stack metadata still remain visible; keep any future narrowing
  semantic rather than hiding needed route facts.
- If value-location output stays too noisy on large functions, treat value
  focus as its own observability seam instead of overloading this block packet.

## Proof

Ran the acceptance subset and spot checks:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_(dump_bir_(focus_block_(entry|missing)_00204|focus_function_filters_00204|00204_stdarg_semantic_handoff)|dump_prepared_bir_(focus_block_(entry|missing)_00204|focus_function_filters_00204|00204_stdarg_prepared_handoff)|dump_mir_focus_function_filters_00204|trace_mir_focus_function_filters_00204|dump_mir_focus_block_(entry|missing)_00204|trace_mir_focus_block_(entry|missing)_00204)|backend_x86_route_debug)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-bir --mir-focus-function stdarg --mir-focus-block entry tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --dump-prepared-bir --mir-focus-function stdarg --mir-focus-block entry tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
and it passed. The proof locks matching and missing block focus on semantic and
prepared dumps, preserves the existing focused MIR coverage, and spot-checks
the real `00204.c` `stdarg` entry block through both dump stages. Proof log
path: `test_after.log`.
