# Parser Public Facade PIMPL Boundary

Status: Open
Created: 2026-04-24
Last Updated: 2026-04-24
Parent Ideas:
- [92_parser_agent_index_header_hierarchy.md](/workspaces/c4c/ideas/closed/92_parser_agent_index_header_hierarchy.md)

## Goal

Make `src/frontend/parser/parser.hpp` a genuinely small public facade by moving
parser-private state ownership and implementation-only method declarations
behind an implementation boundary.

## Why This Idea Exists

Idea 92 moved parser-private headers under `src/frontend/parser/impl/` and
introduced `impl/parser_impl.hpp` as the private implementation index, but the
last public state exposure is larger than a header relocation slice.

`Parser` still stores parser state carriers by value in `parser.hpp`, including
core input, binding, definition, template, lexical-scope, active-context,
namespace, diagnostic, and pragma state. Because those members have complete
types in the public class layout, `parser.hpp` must include
`impl/parser_state.hpp`. Parser implementation files are also class methods
implemented directly over those fields, so removing the include now requires an
object-layout and method-boundary rebuild rather than more accessor helpers.

## Desired Shape

- External callers include `src/frontend/parser/parser.hpp` to construct
  `Parser`, configure public parser options/debug controls, call `parse()`, and
  inspect the supported public result/error surface.
- `parser.hpp` forward-declares or hides parser-private implementation state
  instead of including `impl/parser_state.hpp`.
- Parser implementation files include `impl/parser_impl.hpp` for the complete
  private state and implementation method surface.
- Test-only hooks are clearly named and do not require external callers to
  depend on private state carrier definitions.

## Candidate Direction

Explore a conservative facade/PIMPL boundary:

- move parser-private state into an implementation-owned object, such as
  `ParserImpl` or `ParserState`, stored opaquely by `Parser`
- keep the `Parser` constructor and `parse()` behavior stable
- move split-translation-unit implementation method declarations out of the
  public header where practical
- preserve existing parser behavior, AST output, diagnostics behavior, and
  testcase expectations

## Acceptance Criteria

- `src/frontend/parser/parser.hpp` no longer includes
  `src/frontend/parser/impl/parser_state.hpp`.
- External app, HIR, frontend, and parser-test callers can include
  `parser.hpp` without seeing private parser state carrier definitions.
- Parser implementation files still have one obvious private index:
  `src/frontend/parser/impl/parser_impl.hpp`.
- Public parser behavior remains source-compatible for normal construction,
  debug configuration, `parse()`, and parse-error inspection.
- Focused parser tests pass.
- `c4c_frontend` and `c4cll` build.

## Non-Goals

- no parser semantic changes
- no testcase expectation downgrades
- no AST model split
- no traditional separated `include/` tree
- no one-header-per-parser-`.cpp` convention
- no broad parser algorithm rewrite beyond what the facade boundary requires
