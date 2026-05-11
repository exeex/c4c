Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR struct method rendered maps are now explicitly documented and named as
no-owner rendered compatibility for mangled-name, link-name id, and return-type
lookup. The `HirStructMethodLookupKey` maps remain the authoritative path for
complete owner/method metadata; complete owner-key misses still fail closed
before rendered compatibility while structured base fallback remains intentional.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/impl/lowerer.hpp` | Labeled retained struct method rendered maps as no-owner compatibility and documented owner-key authority plus closed misses. |
| `src/frontend/hir/hir_types.cpp` | Renamed the struct method rendered fallback gates to no-owner compatibility and commented the closed-miss boundary plus rendered map writes. |
| `tests/frontend/frontend_hir_tests.cpp` | Renamed focused struct method tests/messages for no-owner compatibility and complete owner-key miss closure. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Tightened owner-key win diagnostics to call stale rendered method maps no-owner compatibility data. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Supervisor should review whether Step 4 now has enough explicit compatibility
fencing for HIR rendered declarations/templates, or delegate the next narrowly
owned bridge-retirement packet.

## Watchouts

- This packet made no behavior change; the existing owner-key-first control
  flow already closes complete misses before rendered lookup and still searches
  structured bases afterward.
- The retained rendered maps remain writable during lowering for compatibility,
  but comments now describe them as no-owner compatibility rather than lookup
  authority.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  was not touched.
- No current blockers.

## Proof

Proof command, logged to `test_after.log`:

`cmake --build build --target frontend_hir_tests frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$'`

Result: passed; 2/2 focused tests passed (`frontend_hir_tests`,
`frontend_hir_lookup_tests`).

Additional check: `git diff --check` passed.
