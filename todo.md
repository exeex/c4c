# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Make X86 Rejection Diagnostics Plain And Actionable
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2.2 completed for one bounded multi-block compare-driven lane. `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
now rejects multi-block prepared functions with branch/join metadata and more
than one parameter with a stable shape-specific compare-driven message instead
of falling through to the generic ordinary miss, and
`src/backend/mir/x86/codegen/route_debug.cpp` mirrors that lane as
`compare-driven-entry` so `--dump-mir` / `--trace-mir` surface the same final
rejection for `tests/c/external/c-testsuite/src/00204.c` `function match`.
Nearest coverage now locks both the route-debug wording and the public x86 BIR
entry rejection for the same bounded multi-parameter compare-driven shape.

## Suggested Next

Pick the next idea-67 packet that teaches another bounded multi-block x86 lane
to report a durable final rejection before the ordinary miss, ideally one of
the remaining single-parameter non-i32 or large variadic prepared shapes that
still collapse to the generic fallback in `00204.c`.

## Watchouts

- This slice is intentionally limited to multi-parameter compare-driven
  functions with prepared branch/join metadata; single-parameter non-i32
  multi-block shapes still fall through to their existing downstream
  restrictions or the generic miss.
- `00204.c` `match` now lands on `compare-driven-entry` with a shape-specific
  final rejection, but nearby large unsupported functions like `myprintf` still
  end at the ordinary prepared-module miss.
- The new route-debug probe is diagnostic only; it does not widen x86 support
  for the rejected shape.

## Proof

Ran the delegated proof command exactly as assigned:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_x86_(route_debug|handoff_boundary)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir tests/c/external/c-testsuite/src/00204.c > test_after.log 2>&1`
and it passed. `test_after.log` preserves the final `--trace-mir` output, and
the `--dump-mir` summary now shows `function match` ending with
`compare-driven-entry recognized the function, but the prepared shape is
outside the current x86 support`.
