Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Field-Scoped Demotion Slice

# Current Packet

## Just Finished

Step 4 - Field-Scoped Demotion Slice completed for `PreparedBirModule::route`.
Route storage was demoted from the public `route` field to private `route_`;
the accepted read surface remains `prepared_route(const PreparedBirModule&)`.

Compatibility surface preserved:

- Header output continues to route through `prepare_route_name(prepared_route(module))`,
  so the prepared printer contract remains unchanged.
- The stored value still defaults to `PrepareRoute::SemanticBirShared`.
- The only route-specific designated initializer in `BirPreAlloc` was replaced
  with default construction plus assignment to public fields, preserving the
  same prepared module state without reworking unrelated construction.
- `PreparedBirModule::liveness`, `invariants`, `completed_phases`, and `notes`
  remained public analysis fields and were not edited.

Route decision: demoted. No external named consumer required retaining
`PreparedBirModule::route` as public; the remaining access is field-scoped to
the `prepared_route(...)` helper.

## Suggested Next

Proceed to the next plan step selected by the supervisor. Suggested packet:
continue the metadata-gate pass with the next single field or consumer family,
keeping `PreparedBirModule::liveness`, `invariants`, `completed_phases`, and
`notes` out of any route-only follow-up.

## Watchouts

- `PreparedBirModule` is no longer an aggregate because `route_` is private;
  the only observed route-specific designated initializer was the owned
  `BirPreAlloc` construction path and it was updated narrowly.
- Keep future access to the route value through `prepared_route(...)`; do not
  reintroduce direct field access.
- The delegated proof did not include `backend_x86_route_debug`; route-debug
  compatibility was preserved by leaving debug code untouched and keeping the
  prepared dump header path unchanged.

## Proof

Code-changing packet proof passed.

Command:
`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_focus_function_filters_00204)$' --output-on-failure) > test_after.log 2>&1`

Result: build succeeded and all 3 selected tests passed. Proof log:
`test_after.log`.
