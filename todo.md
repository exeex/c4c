# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Completed the local-symbol portion of Step 3.3 by adding structured metadata
for injected predefined locals when their function body or constructor
initializer arguments carry a parser TextId reference. `__func__`,
`__FUNCTION__`, `__PRETTY_FUNCTION__`, and injected `this` now bind through the
same local structured map used by normal declarations when such a carrier is
available.

Added focused `frontend_parser_lookup_authority_tests` coverage showing stale
rendered local references to `__func__` and `this` are rejected after a
structured local key miss when an earlier structured producer carrier exists.

## Suggested Next

Continue Step 3.3 by having the supervisor/reviewer decide whether the
remaining no-metadata local rendered compatibility is acceptable for textless
or synthetic bindings, or whether the route should split any truly textless
producer into a separate metadata-carrier packet.

## Watchouts

- The broad proof still contains the pre-existing `frontend_parser_tests`
  failure: `record-body using member typedef writer should register a direct
  record/member key`. Supervisor-side matching before/after validation confirms
  this packet did not introduce that failure.
- An over-strict first cut also broke namespace/global using-declaration
  bridges; the retained `structured_global_self_names_` guard is required so a
  same-spelling structured global can still satisfy those imports while stale
  rendered globals whose metadata names another symbol remain rejected.
- `c4c-clang-tools` was not needed for this packet; the candidate route was
  localized in `src/frontend/sema/validate.cpp::lookup_symbol()`.
- Injected predefined locals still have no structured key when no matching
  parser TextId reference is present in the function body or constructor
  initializer arguments; the rendered compatibility path remains limited to
  that no-carrier case.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build and 885/885
passing focused tests.
