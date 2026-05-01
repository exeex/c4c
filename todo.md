# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3.2
Current Step Title: Add Qualifier-Aware Sema Global/Enum Metadata Keys

## Just Finished

Step 3.3.2 implementation: added qualifier-aware Sema global/enum lookup keys
and rendered-metadata guards so same-member/wrong-owner qualified global and
enum references no longer recover through broad rendered compatibility after
structured metadata exists. Added focused global and enum authority tests for
that drift shape.

## Suggested Next

Supervisor should review/accept the completed Step 3.3.2 slice and decide
whether Step 3.3 now needs route review, closure, or the next source-idea
packet.

## Watchouts

- The broad proof still contains the pre-existing `frontend_parser_tests`
  failure: `record-body using member typedef writer should register a direct
  record/member key`. Supervisor-side matching before/after validation confirms
  this packet did not introduce that failure.
- The global path intentionally preserves namespace/global using-declaration
  bridges: same-spelling structured globals can still satisfy those imports,
  while qualified rendered globals whose stored qualifier metadata disagrees
  with the reference qualifier are rejected.
- `c4c-clang-tools` was not needed for this packet; the candidate route was
  localized in `src/frontend/sema/validate.cpp::lookup_symbol()`.
- Injected predefined locals still have no structured key when no matching
  parser TextId reference is present in the function body or constructor
  initializer arguments; the rendered compatibility path remains limited to
  that no-carrier case.
- Enum-domain metadata is carried from the enum declaration qualifier into the
  global enum-constant key when present; unqualified enum lookup keeps the
  existing broad key path.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Passed. `test_after.log` is the canonical proof log.
