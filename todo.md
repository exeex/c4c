Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Accessor Or Adapter Shape

# Current Packet

## Just Finished

Step 3 - Accessor Or Adapter Shape completed for `PreparedBirModule::route`.
The selected shape is the inline read-only accessor
`prepared_route(const PreparedBirModule&)`, with `prepare::print` reading the
route through that accessor before calling `prepare_route_name(...)`.

Compatibility surface preserved:

- Header output remains `prepared.module target=<triple>
  route=semantic_bir_shared`; no printer text, route names, or fallback route
  behavior changed.
- The accessor returns the stored `PrepareRoute` value unchanged, so invalid
  enum values still flow through `prepare_route_name(...) == "unknown"` rather
  than gaining a new interpretation.
- Debug compatibility remains indirect through the prepared dump surface used
  by existing route-debug coverage; no x86 debug text was changed.
- `PreparedBirModule::liveness`, `invariants`, `completed_phases`, and `notes`
  remained analysis-only and were not edited.

Route decision: ready for a later field-scoped demotion slice. The remaining
direct public-field reliance for compatibility output is now narrowed to
`prepared_route(...)`; the storage field itself remains public until Step 4
decides whether to demote or retain it.

## Suggested Next

Proceed to Step 4 with a route-only field-scoped demotion or retained-public
decision. Suggested packet: decide whether `PreparedBirModule::route` can move
behind the accepted `prepared_route(...)` access point, while preserving
aggregate construction and the prepared printer header contract exactly.

## Watchouts

- `PreparedBirModule::route` still appears in aggregate initialization and as
  the accessor implementation detail; Step 4 should avoid breaking designated
  initialization unless it intentionally changes construction shape.
- Keep `PreparedBirModule::liveness` out of scope.
- Do not weaken CLI required snippets or route-debug assertions to make a route
  demotion compile; preserve compatibility through `prepared_route(...)`.
- The delegated proof did not include `backend_x86_route_debug`; route-debug
  compatibility was preserved by leaving debug code untouched and keeping the
  prepared dump header byte-for-byte.

## Proof

Code-changing packet proof passed.

Command:
`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_focus_function_filters_00204)$' --output-on-failure) > test_after.log 2>&1`

Result: build succeeded and all 3 selected tests passed. Proof log:
`test_after.log`.
