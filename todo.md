Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Public Parser Surface

# Current Packet

## Just Finished

Completed Step 1 inventory for the parser public facade/PIMPL boundary without
editing implementation files.

True public parser API:
- `src/frontend/parser/parser.hpp` production callers need the constructor,
  `parse()`, parse-debug channel constants, `set_parser_debug_channels()`, and
  `had_parse_error()`.
- Production direct `c4c::Parser` use is limited to `src/apps/c4cll.cpp` and
  `tests/frontend/frontend_hir_tests.cpp`; other `#include "parser.hpp"` hits in
  backend assembler paths are local assembler parser headers, not the frontend
  facade.

Private implementation-only surface currently exposed through the facade:
- State carriers from `src/frontend/parser/impl/parser_state.hpp` are public
  members or aliases in `Parser`: `ParserCoreInputState`,
  `ParserSharedLookupState`, `ParserBindingState`, `ParserDefinitionState`,
  `ParserTemplateState`, `ParserLexicalScopeState`,
  `ParserActiveContextState`, `ParserNamespaceState`,
  `ParserDiagnosticState`, and `ParserPragmaState`.
- Implementation helper families in `Parser` operate directly on those state
  carriers: token cursor/mutation, diagnostics and tentative parse snapshots,
  namespace/name lookup, typedef/value binding, template metadata, qualified
  names, declarators, record bodies, expressions, statements, and AST builders.
- `src/frontend/parser/impl/parser_impl.hpp` is the private implementation
  index and is included only by parser implementation translation units; it
  currently adds free helper declarations such as enum-expression evaluation,
  scalar alignment, qualifier/storage/type keyword predicates, and literal
  lexeme parsing.

Required opaque ownership special members for the next boundary:
- Because `Parser` will own an incomplete implementation object, the facade will
  need out-of-line ownership special members in the public header, at minimum a
  destructor plus deleted copy construction/copy assignment.
- Move construction/move assignment should be decided when Step 2 chooses the
  actual opaque owner type; avoid exposing `impl/parser_state.hpp` just to make
  the owner destructible inline.

Test-only caller dependencies to preserve or isolate:
- `tests/frontend/frontend_parser_tests.cpp` reaches many parser internals:
  symbol IDs/tables, snapshot restore, token injection and cursor inspection,
  typedef/value/concept/namespace/template registration, qualified-name parsing,
  declarator/type/expression/statement entry points, and AST builders.
- Test-only names already marked `*_for_testing` should remain isolated; many
  current test dependencies are not yet marked that way and should move behind a
  named testing access layer or remain explicitly test-only during later steps.

## Suggested Next

Execute Step 2 in `plan.md`: introduce opaque parser implementation ownership in
the facade, add the required ownership special members, and keep behavior
unchanged.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- Do not expose `impl/parser_state.hpp` through the public facade.
- Keep implementation-only declarations behind
  `src/frontend/parser/impl/parser_impl.hpp` where practical.
- Keep any test-only hooks clearly named and isolated.
- `parser.hpp` currently includes `impl/parser_state.hpp` solely to make the
  public `Parser` layout, aliases, snapshots, guards, and test/helper signatures
  complete; Step 2 should not try to move every helper at once.
- The facade/PIMPL split must not confuse frontend `parser.hpp` with backend
  assembler-local `parser.hpp` include hits.

## Proof

Read-only inventory proof captured in `test_after.log`:
- `rg -n "#include .*parser/parser\\.hpp|Parser\\b|ParserOptions|parse\\(|Parse" src tests`
- direct reads of `src/frontend/parser/parser.hpp`,
  `src/frontend/parser/impl/parser_impl.hpp`, and
  `src/frontend/parser/impl/parser_state.hpp`
- follow-up include/caller narrowing with `rg` over `src` and `tests`

No build was run because this packet changed only `todo.md`.
