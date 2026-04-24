Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Clean Compatibility Fallout And Documentation

# Current Packet

## Just Finished

Completed plan Step 4 by cleaning stale comments, docs, and parser-focused
test comments left behind by the Step 2/3 file moves.

Updated live references that still implied parser implementation files lived at
the old top-level paths:
- `src/frontend/parser/parser.hpp` now points implementation navigation at the
  `impl/` and `impl/types/` files and keeps the public facade role explicit.
- `src/frontend/parser/impl/parser_impl.hpp` now describes itself as the
  private index for `impl/*.cpp` translation units.
- `src/frontend/parser/BOUNDARY_AUDIT.md` now names the moved parser
  implementation paths.
- `docs/frontend_compile_time_hotspot_inventory.md` now preserves the original
  timing data while naming the moved parser translation units.
- Parser-focused regression comments that named `parser_types_struct.cpp` now
  name `impl/types/struct.cpp`.

Lightweight stale-reference search:

```sh
rg -n "parser_(core|declarations|expressions|statements|support)\\.cpp|parser_types_(base|declarator|struct|template)\\.cpp|src/frontend/parser/(parser_(core|declarations|expressions|statements|support)|parser_types_(base|declarator|struct|template))\\.cpp|src/frontend/parser/(declarations|expressions|statements|types|support|core)\\.cpp" --glob '!plan.md' --glob '!todo.md' --glob '!ideas/**'
```

Result: no matches in live source, docs, tests, build files, or scripts outside
the lifecycle/source-idea/archive paths intentionally excluded above.

## Suggested Next

Ask the plan owner to decide whether the active runbook is exhausted and should
close, retire, or continue with a follow-up packet.

## Watchouts

- Keep the transient `review/step3_parser_facade_boundary_review.md` artifact
  out of lifecycle commits unless the supervisor explicitly requests it.
- The stale-reference search intentionally excluded `plan.md`, `todo.md`, and
  `ideas/**` because the active plan/source idea record the historical move map
  and the repository treats closed/draft ideas as archive.
- No parser semantics, build sources, file moves, `ast.hpp` split, or
  one-header-per-implementation-file index work was done in this packet.

## Proof

Delegated proof passed:

```sh
{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`.
