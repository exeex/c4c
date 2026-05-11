Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire LIR and HIR-to-LIR Final-Output Bridges

# Current Packet

## Just Finished

Step 5 - Retire LIR and HIR-to-LIR Final-Output Bridges:
Fenced dead-internal global-initializer reachability so complete
`LirGlobal::initializer_function_link_name_ids` metadata closes the raw
`init_text` fallback. `eliminate_dead_internals` now seeds from valid
initializer `LinkNameId`s and scans final initializer spelling only when a
global has no valid structured initializer function IDs.

Added focused LIR coverage proving stale `init_text` does not keep a
rendered-name shadow alive when structured initializer IDs exist, while
legacy/no-metadata initializer payloads still scan raw `init_text`.

Changed files:

| File | Result |
| --- | --- |
| `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` | Made valid initializer function `LinkNameId`s authoritative before raw initializer text scanning in dead-internal elimination. |
| `tests/frontend/frontend_hir_tests.cpp` | Added direct LIR reachability coverage for stale `init_text` fencing and legacy no-metadata fallback preservation. |
| `todo.md` | Recorded this Step 5 packet result and proof. |

## Suggested Next

Continue Step 5 by inspecting the next LIR/HIR-to-LIR final-output bridge that
still recovers semantic reachability or identity from rendered LLVM text despite
available structured IDs or typed mirrors.

## Watchouts

- `LirGlobal::init_text` remains final output payload and is still scanned for
  legacy initializer producers with no valid structured function IDs.
- The focused test exposes `eliminate_dead_internals` to the test TU through a
  namespace-local forward declaration rather than a public header declaration.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Proof command:
`cmake --build build --target frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result: passed. Output was written to `test_after.log`.

Also ran `git diff --check`: passed.
