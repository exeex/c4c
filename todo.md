Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Type Parser Implementation Files Under impl/types/

# Current Packet

## Just Finished

Completed plan Step 3 by moving the four type-family parser implementation
files under `src/frontend/parser/impl/types/` and removing redundant
`parser_types_` filename prefixes:
- `src/frontend/parser/parser_types_base.cpp` ->
  `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp` ->
  `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp` ->
  `src/frontend/parser/impl/types/struct.cpp`
- `src/frontend/parser/parser_types_template.cpp` ->
  `src/frontend/parser/impl/types/template.cpp`

Updated only the includes made stale by the new directory depth:
- Moved type-family files now include `"../parser_impl.hpp"` instead of
  `"impl/parser_impl.hpp"`.
- Moved type-family files now include `"types_helpers.hpp"` instead of
  `"impl/types/types_helpers.hpp"`.

No CMake source-list edit was needed because the frontend target uses a
recursive `*.cpp` glob.

## Suggested Next

Start Step 4 cleanup by searching for stale references to the old top-level
parser implementation filenames in public comments, docs, tests, and source
navigation notes, then update only references that are still inaccurate after
the moves.

## Watchouts

- This is structural parser layout work; preserve parser behavior, AST output,
  diagnostics behavior, visible-name behavior, pragma behavior, template
  parsing behavior, and testcase expectations.
- Do not split `ast.hpp`.
- Do not create one-header-per-implementation-file indexes.
- The five non-type implementation files now live under `impl/`, and the four
  type-family implementation files now live under `impl/types/`; keep them
  there in later cleanup.
- The previous parser facade boundary work already made
  `src/frontend/parser/parser.hpp` a smaller public surface; do not regress it
  by reintroducing private state dependencies.
- Keep the transient `review/step3_parser_facade_boundary_review.md` artifact
  out of lifecycle commits unless the supervisor explicitly requests it.
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
