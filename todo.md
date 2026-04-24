Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Core Parser Implementation Files Under impl/

# Current Packet

## Just Finished

Completed plan Step 2 by moving the five non-type parser implementation files
under `src/frontend/parser/impl/` and removing redundant `parser_` filename
prefixes:
- `src/frontend/parser/parser_core.cpp` ->
  `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/parser_declarations.cpp` ->
  `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp` ->
  `src/frontend/parser/impl/expressions.cpp`
- `src/frontend/parser/parser_statements.cpp` ->
  `src/frontend/parser/impl/statements.cpp`
- `src/frontend/parser/parser_support.cpp` ->
  `src/frontend/parser/impl/support.cpp`

Updated only the includes made stale by the new directory depth:
- Moved files now include `"parser_impl.hpp"` instead of
  `"impl/parser_impl.hpp"`.
- Moved expression, statement, and declaration implementations that use type
  helpers now include `"types/types_helpers.hpp"` instead of
  `"impl/types/types_helpers.hpp"`.
- The declaration implementation's macro guards around `types_helpers.hpp`
  were preserved.

No CMake source-list edit was needed because the frontend target uses a
recursive `*.cpp` glob.

## Suggested Next

Start Step 3 by moving the four type-family implementation files under
`src/frontend/parser/impl/types/` and retargeting their local private includes.

## Watchouts

- This is structural parser layout work; preserve parser behavior, AST output,
  diagnostics behavior, visible-name behavior, pragma behavior, template
  parsing behavior, and testcase expectations.
- Do not split `ast.hpp`.
- Do not create one-header-per-implementation-file indexes.
- The five non-type implementation files now live under `impl/`; keep them
  there in later cleanup.
- The previous parser facade boundary work already made
  `src/frontend/parser/parser.hpp` a smaller public surface; do not regress it
  by reintroducing private state dependencies.
- Keep the transient `review/step3_parser_facade_boundary_review.md` artifact
  out of lifecycle commits unless the supervisor explicitly requests it.
- Step 3 include retargeting: files moved to `impl/types/` should include
  `"../parser_impl.hpp"` and `"types_helpers.hpp"` rather than preserving
  parser-root-relative private paths.
- `parser.hpp` still contains an implementation-map comment listing old flat
  filenames; update or remove that public-facade private-navigation comment in
  a cleanup slice so the public facade does not become stale.
- `BOUNDARY_AUDIT.md` already mentions older post-move-looking names such as
  `declarations.cpp`, `types.cpp`, and `statements.cpp`; treat it as stale
  documentation to reconcile in Step 4 rather than as a build/source reference.

## Proof

Delegated proof passed:

```sh
{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`.
