Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Compatibility Contract Map

# Current Packet

## Just Finished

Step 2 - Compatibility Contract Map completed for `PreparedBirModule::route`,
`invariants`, `completed_phases`, and `notes`. `PreparedBirModule::liveness`
remained out of scope.

Compatibility rows to preserve:

- Route output: `prepare::print` emits the header row
  `prepared.module target=<triple> route=semantic_bir_shared`. Evidence:
  `tests/backend/bir/backend_x86_route_debug_test.cpp` checks generic prepared
  dump equality plus that exact header, and multiple
  `tests/backend/bir/CMakeLists.txt` prepared dump cases require the same
  header in normal and focused output.
- Invariant output: `prepare::print` emits `invariants:` followed by
  `  - no_target_facing_i1` and/or `  - no_phi_nodes` when present. Evidence:
  `tests/backend/bir/backend_prepare_phi_materialize_test.cpp` directly checks
  publication and stable names for both invariants. Missing evidence: no
  printer/CLI test currently asserts the rendered `invariants:` rows.
- Completed-phase text/status row: `prepare::print` emits
  `completed_phases: legalize stack_layout liveness out_of_ssa regalloc` in
  pass order when populated. Evidence:
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff` requires that
  exact row.
- Notes text/status rows: `prepare::print` emits `notes:` followed by rows such
  as `  - [stack_layout] stack layout prepared function 'stdarg'` and
  `  - [regalloc] regalloc seeded function 'stdarg'`. Evidence:
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff` and
  `backend_cli_dump_prepared_bir_focus_function_filters_00204` require those
  note snippets; `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  directly checks the unresolved-PIC diagnostic text in `prepared.notes`.
- Absent-note fallback: the current compatibility contract is omission, not a
  placeholder row: `prepare::print` prints no `notes:` section when
  `module.notes` is empty. Missing evidence: no focused fail-closed test
  currently asserts that an empty note vector omits the section or that a
  fallback state does not synthesize a note.
- Debug/status strings: `backend_x86_route_debug` preserves the split between
  generic prepared dumps and x86 route debug surfaces, including
  `route=semantic_bir_shared`, `x86 route summary`, `route owner: x86/debug`,
  `module emitter: x86/module`, and
  `status: contract-first debug surface; structured lane diagnostics still deferred`.

Missing fail-closed coverage relevant to these fields:

- Invalid route enum values fall through to `prepare_route_name(...) ==
  "unknown"`, but no test exercises an invalid `PrepareRoute` value or asserts
  that the printer/debug surface fails closed instead of emitting a misleading
  supported route.
- Invalid invariant enum values fall through to
  `prepared_bir_invariant_name(...) == "unknown"`, but no test exercises an
  invalid `PreparedBirInvariant` value or asserts the rendered invariant row.
- Mismatched metadata states are not directly fenced: no test checks a
  completed phase without the corresponding invariant or note, an invariant
  without its completed phase, or a note whose phase text is absent from
  `completed_phases`.
- Absent states are only partially covered: empty `completed_phases`,
  `invariants`, and `notes` omit their sections by implementation behavior, but
  focused tests do not assert those omissions.
- Fallback/status coverage is strongest for x86 debug strings and the
  unresolved-PIC note payload, but weak for printer-level fallback rows for
  route, invariant, completed-phase, and empty-note states.

## Suggested Next

Proceed to Step 3 by defining the smallest private pass-context accessor or
adapter shape for the first implementation slice. Suggested first slice:
`route` plus printer/debug compatibility only, because it has one enum value,
one header row, and no direct target-facing public consumer from Step 1.

Smallest focused proof command for that first slice:

`cmake --build build --target c4cll backend_prepared_printer_test backend_x86_route_debug_test && ctest --test-dir build -R '^(backend_prepared_printer|backend_x86_route_debug|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_focus_function_filters_00204)$' --output-on-failure`

If `C4C_ENABLE_X86_BACKEND_TESTS` is off in the active build, drop only
`backend_x86_route_debug_test` and `backend_x86_route_debug`; keep the printer
and CLI prepared-dump cases.

## Watchouts

- Keep `PreparedBirModule::liveness` out of scope.
- Do not weaken CLI required snippets or direct invariant/note assertions to
  make demotion compile; add or redirect through the accepted compatibility
  surface instead.
- Before demoting `invariants`, add or preserve printer-level proof for the
  rendered `invariants:` rows; current direct tests prove publication and names,
  not the printed rows.
- Before demoting `notes`, preserve both note payload access and the empty-notes
  omission contract; current direct tests prove one diagnostic payload, while
  CLI tests prove populated note snippets.
- Before demoting `completed_phases`, preserve phase ordering exactly and avoid
  treating a successful pass run as proof that mismatched metadata states are
  fail-closed.

## Proof

Analysis-only packet; no build or test proof required by the supervisor.
Validation command: `git diff --check -- todo.md`.
