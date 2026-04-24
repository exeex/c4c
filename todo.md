Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Header Boundaries

# Current Packet

## Just Finished

- Completed plan Step 1 inventory for parser header boundaries.
- `parser.hpp` currently mixes the public parser entry API with the private
  implementation index:
  - Public parser API: `Parser` construction and `parse()` at
    `parser.hpp:147-156`.
  - Externally observed parser/debug API: `ParseDebugChannel`,
    `set_parser_debug*`, and `diagnostic_state_` are used by `src/apps/c4cll.cpp`;
    `SymbolId`, `kInvalidSymbol`, `symbol_id_for_token`,
    `symbol_spelling`, `QualifiedNameRef`, `parse_qualified_name`,
    `TemplateScopeKind`, `TemplateArgParseResult`, and several parser tables are
    used by `tests/frontend/frontend_parser_tests.cpp`.
  - Parser-private state: `core_input_state_`, `tokens_`, `pos_`, `arena_`,
    `shared_lookup_state_`, `binding_state_`, `definition_state_`,
    `template_state_`, `lexical_scope_state_`, `active_context_state_`,
    `namespace_state_`, `diagnostic_state_`, and `pragma_state_` at
    `parser.hpp:158-190`.
  - Split-implementation method declarations: token cursor, classification,
    binding, namespace lookup, type/declarator/record/enum/expression/statement,
    declaration, and AST-builder declarations at `parser.hpp:231-882`.
  - Debug plumbing: debug channel enum and parse-debug/failure/tentative
    tracing declarations at `parser.hpp:117-123` and `parser.hpp:197-229`.
  - Lookup tables: parser symbol/name aliases, visible-name result, typedef,
    value, function, concept, namespace, and visible-name lookup declarations at
    `parser.hpp:51-100` and `parser.hpp:321-502`.
  - Template/type helpers: template-scope, template-struct, template-argument,
    type-spelling, type-specifier, declarator, record, enum, and type utility
    declarations at `parser.hpp:102-138`, `parser.hpp:512-839`, and
    `parser.hpp:887-910`.
  - AST-facing data: `Node*` parse/build return surfaces, `TypeSpec`-based
    parse/type helper APIs, and top-level const-eval/type-size helpers at
    `parser.hpp:149-156`, `parser.hpp:614-882`, and `parser.hpp:885-910`.
- `parser_state.hpp` is parser-private state and AST-facing carrier data, not a
  standalone public parser API:
  - Lookup tables and identity model: `ParserSymbolTable`, `ParserNameTables`,
    `ParserSymbolId`, `ParserTokenMutation`, and `ParserFnPtrTypedefInfo` at
    `parser_state.hpp:24-114`.
  - AST/name-facing parser data: `ParserNamespaceContext`,
    `ParserQualifiedNameRef`, `ParserTemplateArgParseResult`,
    `ParserTemplateScope*`, `ParserInjectedTemplateParam`,
    `ParserRecordBodyState`, `ParserRecordMemberRecoveryResult`, and
    `ParserAliasTemplateInfo` at `parser_state.hpp:116-287`.
  - Parser-private state tables: `ParserCoreInputState`,
    `ParserSharedLookupState`, `ParserBindingState`, `ParserDefinitionState`,
    `ParserTemplateState`, `ParserLexicalScopeState`,
    `ParserActiveContextState`, `ParserNamespaceState`,
    `ParserDiagnosticState`, and `ParserPragmaState` at
    `parser_state.hpp:159-469`.
  - Debug/tentative guard plumbing: parse context/failure/event, snapshots,
    tentative stats, context guard, tentative guards, local binding suppression,
    record prelude guard, and template prelude guard at
    `parser_state.hpp:236-277` and `parser_state.hpp:414-529`.
- `types_helpers.hpp` is parser-private type/template helper implementation:
  all definitions live in an anonymous namespace and the file comment says it is
  included by parser type implementation translation units with internal
  linkage. It contains qualified-name/type-head helpers, template argument
  token skipping, injected-parse helpers, qualified type probes, record-member
  boundary helpers, alignas/virt/explicit consumers, template specialization
  matching, template instance key generation, and C++20 requires/member-boundary
  helpers.
- Include-site inventory:
  - External frontend callers of frontend `parser.hpp`:
    `src/apps/c4cll.cpp`, `tests/frontend/frontend_parser_tests.cpp`, and
    `tests/frontend/frontend_hir_tests.cpp`.
  - Parser implementation callers of frontend `parser.hpp`:
    `parser_core.cpp`, `parser_declarations.cpp`, `parser_expressions.cpp`,
    `parser_statements.cpp`, `parser_support.cpp`, `parser_types_base.cpp`,
    `parser_types_declarator.cpp`, `parser_types_struct.cpp`, and
    `parser_types_template.cpp`.
  - `parser_state.hpp` include sites: only `src/frontend/parser/parser.hpp`.
  - `types_helpers.hpp` include sites: `parser_declarations.cpp`,
    `parser_expressions.cpp`, `parser_statements.cpp`,
    `parser_types_base.cpp`, `parser_types_declarator.cpp`,
    `parser_types_struct.cpp`, and `parser_types_template.cpp`.
  - Backend assembler `#include "parser.hpp"` hits are local backend headers
    (`src/backend/mir/riscv/assembler/parser.hpp` and
    `src/backend/mir/aarch64/assembler/parser.hpp`), not frontend parser
    include sites.

## Suggested Next

- First code-changing sub-slice: introduce
  `src/frontend/parser/impl/parser_impl.hpp` as a private parser implementation
  index, update parser implementation `.cpp` files to include it, and leave
  external callers on `parser.hpp`.
- Keep the first movement conservative: make `impl/parser_impl.hpp` include
  `../parser.hpp` as a bridge first, then move parser-private declarations out
  of `parser.hpp` only after parser implementation files compile through the
  private index.

## Watchouts

- Keep this initiative structural; do not change parser semantics, visible-name
  behavior, AST output, or testcase expectations.
- Idea 93 depends on this parser header hierarchy work and should remain
  inactive until idea 92 is complete or intentionally split.
- `tests/frontend/frontend_parser_tests.cpp` reaches into many current
  implementation details (`tokens_`, binding tables, template scopes,
  qualified-name parsing, and visible/template helper methods). Treat these as
  external include-site hazards for Step 2: they may need temporary access to
  `impl/parser_impl.hpp` or test-scope retargeting before `parser.hpp` can be
  minimal.
- `src/apps/c4cll.cpp` needs debug channel constants, `set_parser_debug_channels`,
  `parse()`, and `diagnostic_state_.had_error`; moving `diagnostic_state_`
  behind the private boundary will require either a public accessor or an app
  retarget in a later slice.
- `types_helpers.hpp` depends on parser implementation internals and is
  included from `parser_declarations.cpp`, `parser_expressions.cpp`, and
  `parser_statements.cpp` as well as type parser files, so the later
  `impl/types/` move is broader than the file comment's type-only list.
- Root-level backend assembler include hits named `parser.hpp` are unrelated
  local headers; avoid retargeting them during frontend parser include work.

## Proof

- No build/test run, per delegated inventory-only proof.
- Inspected headers with:
  `sed -n '1,260p' src/frontend/parser/parser.hpp`,
  `sed -n '261,620p' src/frontend/parser/parser.hpp`,
  `sed -n '621,980p' src/frontend/parser/parser.hpp`,
  `sed -n '1,260p' src/frontend/parser/parser_state.hpp`,
  `sed -n '261,620p' src/frontend/parser/parser_state.hpp`,
  `sed -n '1,260p' src/frontend/parser/types_helpers.hpp`,
  `sed -n '261,620p' src/frontend/parser/types_helpers.hpp`,
  `sed -n '621,980p' src/frontend/parser/types_helpers.hpp`,
  and targeted `nl -ba` ranges for line references.
- Inspected include sites with:
  `rg -n '#include "(src/frontend/parser/)?(parser\\.hpp|parser_state\\.hpp|types_helpers\\.hpp)"|#include <(src/frontend/parser/)?(parser\\.hpp|parser_state\\.hpp|types_helpers\\.hpp)>' .`,
  `rg -n '#include "parser\\.hpp"|#include "parser_state\\.hpp"|#include "types_helpers\\.hpp"' src/frontend/parser src/apps tests/frontend -g'*.cpp' -g'*.hpp'`,
  `rg --files | rg '(^|/)parser\\.hpp$|(^|/)parser_state\\.hpp$|(^|/)types_helpers\\.hpp$'`,
  and targeted reads of backend assembler local `parser.hpp` files to classify
  name collisions.
- Proof rationale: inventory-only packet touched no implementation, tests,
  build artifacts, or include paths, so compile/test proof was not required and
  no `test_after.log` was created.
