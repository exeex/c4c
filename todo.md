Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Layout And Build References

# Current Packet

## Just Finished

Completed Step 1 inventory for the parser layout move set.

Current parser file classification under `src/frontend/parser/`:
- Public/facade headers: `ast.hpp`, `parser.hpp`, `parser_support.hpp`,
  `parser_types.hpp`.
- Parser-private headers: `impl/parser_impl.hpp`, `impl/parser_state.hpp`.
- Type-family private helper header:
  `impl/types/types_helpers.hpp`.
- Non-type implementation files at parser top-level:
  `parser_core.cpp`, `parser_declarations.cpp`,
  `parser_expressions.cpp`, `parser_statements.cpp`,
  `parser_support.cpp`.
- Type-family implementation files at parser top-level:
  `parser_types_base.cpp`, `parser_types_declarator.cpp`,
  `parser_types_struct.cpp`, `parser_types_template.cpp`.
- Parser-local documentation/audit file:
  `BOUNDARY_AUDIT.md`.

Concrete move set recorded for Step 2/3:
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
- `src/frontend/parser/parser_types_base.cpp` ->
  `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp` ->
  `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp` ->
  `src/frontend/parser/impl/types/struct.cpp`
- `src/frontend/parser/parser_types_template.cpp` ->
  `src/frontend/parser/impl/types/template.cpp`

Build/source reference inventory:
- `src/frontend/CMakeLists.txt` uses
  `file(GLOB_RECURSE C4C_FRONTEND_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")`; there is no hand-maintained parser
  source list to update for the frontend target.
- Parser include directory remains exported by `src/frontend/CMakeLists.txt`
  via `"${CMAKE_CURRENT_SOURCE_DIR}/parser"`, so include sites such as
  `"parser.hpp"` and `"impl/parser_impl.hpp"` continue to resolve after moving
  implementation files as long as private relative includes are retargeted.
- `src/backend/CMakeLists.txt` also exports
  `"${PROJECT_SOURCE_DIR}/src/frontend/parser"` for backend-dependent routes;
  no parser `.cpp` source list is named there.
- Current old-filename textual references that will need refresh or deliberate
  preservation after moves:
  `src/frontend/parser/parser.hpp` implementation-map comments,
  `src/frontend/parser/impl/parser_impl.hpp` private-index comment,
  top-of-file comments in `parser_types_base.cpp`,
  `parser_types_declarator.cpp`, `parser_types_struct.cpp`, and
  `parser_types_template.cpp`,
  `docs/frontend_compile_time_hotspot_inventory.md`,
  and parser-focused testcase comments in
  `tests/cpp/internal/postive_case/runtime/record_template_enum_underlying_typedef_parse.cpp`
  and
  `tests/cpp/internal/postive_case/runtime/record_template_prelude_and_tag_registration_parse.cpp`.
- Planned destination filenames are currently referenced only by the active
  source idea/runbook and stale audit-style text such as
  `src/frontend/parser/BOUNDARY_AUDIT.md`; no source, script, test, or CMake
  file currently names the new destination `.cpp` paths as build inputs.

Facade/private-index confirmation:
- `parser.hpp` is still the public parser facade: it includes public parser
  surfaces (`ast.hpp`, `parser_types.hpp`) and shared public dependencies, and
  forward-declares parser-private state carriers instead of including
  `impl/parser_state.hpp`.
- `impl/parser_impl.hpp` is usable as the private implementation index: all
  nine current parser implementation files include it, and it includes
  `../parser_support.hpp`, `../parser.hpp`, and `parser_state.hpp`.
- `tests/frontend/frontend_parser_tests.cpp` intentionally includes
  `impl/parser_impl.hpp` for parser-private hooks; keep that path usable.

## Suggested Next

Start Step 2 by moving the five non-type implementation files under
`src/frontend/parser/impl/`, updating their local includes and allowing the
frontend CMake glob to pick up the renamed files.

## Watchouts

- This is structural parser layout work; preserve parser behavior, AST output,
  diagnostics behavior, visible-name behavior, pragma behavior, template
  parsing behavior, and testcase expectations.
- Do not split `ast.hpp`.
- Do not create one-header-per-implementation-file indexes.
- Do not keep implementation `.cpp` files at parser top-level once their
  semantic `impl/` directory exists.
- The previous parser facade boundary work already made
  `src/frontend/parser/parser.hpp` a smaller public surface; do not regress it
  by reintroducing private state dependencies.
- Keep the transient `review/step3_parser_facade_boundary_review.md` artifact
  out of lifecycle commits unless the supervisor explicitly requests it.
- Step 2 include retargeting: files moved to `impl/` should include
  `"parser_impl.hpp"` instead of `"impl/parser_impl.hpp"`; files that use type
  helpers from `impl/` should include `"types/types_helpers.hpp"` instead of
  `"impl/types/types_helpers.hpp"`.
- Step 3 include retargeting: files moved to `impl/types/` should include
  `"../parser_impl.hpp"` and `"types_helpers.hpp"` rather than preserving
  parser-root-relative private paths.
- `parser_declarations.cpp` currently wraps `types_helpers.hpp` with macro
  renames before inclusion; preserve those macro guards when moving it.
- `parser.hpp` still contains an implementation-map comment listing old flat
  filenames; update or remove that public-facade private-navigation comment in
  a cleanup slice so the public facade does not become stale.
- `BOUNDARY_AUDIT.md` already mentions older post-move-looking names such as
  `declarations.cpp`, `types.cpp`, and `statements.cpp`; treat it as stale
  documentation to reconcile in Step 4 rather than as a build/source reference.

## Proof

Inventory-only lifecycle scratchpad work; no build or test proof required by
the delegated packet.

Lightweight searches run:
- `find src/frontend/parser -maxdepth 3 -type f | sort`
- `rg -n "parser_(core|declarations|expressions|statements|support|types_base|types_declarator|types_struct|types_template)\.cpp|impl/(core|declarations|expressions|statements|support)\.cpp|impl/types/(base|declarator|struct|template)\.cpp" .`
- `rg -n "parser_core\.cpp|parser_declarations\.cpp|parser_expressions\.cpp|parser_statements\.cpp|parser_support\.cpp|parser_types_base\.cpp|parser_types_declarator\.cpp|parser_types_struct\.cpp|parser_types_template\.cpp|impl/core\.cpp|impl/declarations\.cpp|impl/expressions\.cpp|impl/statements\.cpp|impl/support\.cpp|impl/types/base\.cpp|impl/types/declarator\.cpp|impl/types/struct\.cpp|impl/types/template\.cpp" CMakeLists.txt src tests docs scripts ideas/open/92_parser_agent_index_header_hierarchy.md`
- `rg -n "#include \"impl/parser_impl\.hpp\"|#include \"impl/types/types_helpers\.hpp\"|#include \"\.\./parser\.hpp\"|#include \"\.\./parser_support\.hpp\"|#include \"\.\./\.\./parser" src/frontend/parser tests/frontend`
