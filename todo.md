Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR ref-overload rendered maps are now explicitly fenced as legacy
no-owner/no-decl compatibility surfaces. The structured owner-key and decl-key
maps remain authoritative; complete owner-key and decl-key misses still fail
closed before rendered compatibility.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/impl/lowerer.hpp` | Documented `ref_overload_set_` as legacy no-owner/no-decl compatibility and the owner/decl maps as authoritative. |
| `src/frontend/hir/impl/expr/operator.cpp` | Renamed the struct ref-overload rendered gate to no-owner compatibility and documented owner-key/decl-key closed-miss boundaries. |
| `src/frontend/hir/hir_build.cpp` | Documented rendered ref-overload map writes as no-decl compatibility mirrored into the authoritative decl-key map. |
| `tests/frontend/frontend_hir_tests.cpp` | Renamed focused free/method ref-overload tests and messages around no-decl/no-owner compatibility and closed misses. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Supervisor should decide whether Step 4 has enough HIR rendered compatibility
fencing to move to review/closure, or delegate the next narrowly owned
bridge-retirement packet.

## Watchouts

- This packet made no behavior change; the existing owner-key and decl-key
  paths already close complete misses before rendered compatibility.
- `resolve_ref_overload` still directly reads the rendered map for callers that
  route by rendered base name; comments now identify that path as legacy
  no-decl compatibility.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  was not touched.
- No current blockers.

## Proof

Proof command, logged to `test_after.log`:

`cmake --build build --target frontend_hir_tests frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$'`

Result: passed; 2/2 focused tests passed (`frontend_hir_tests`,
`frontend_hir_lookup_tests`).

Additional check: `git diff --check` passed.
