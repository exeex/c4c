# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split Parser Semantic Lookup From Text Spelling

## Just Finished

Completed Step 2 parser-template mirror demotion check for the
`ParserTemplateState` family.

Verified the owned template paths keep structured/TextId authority ahead of
rendered mirror entries:
- `find_template_struct_primary` and `find_template_struct_specializations`
  use `QualifiedNameKey` lookups for valid `TextId` names and only consult
  rendered `template_struct_defs`/`template_struct_specializations` when the
  lookup is TextId-less or otherwise a compatibility bridge.
- `eval_deferred_nttp_default` uses `NttpDefaultExprKey` results for valid
  template text and treats rendered `nttp_default_expr_tokens` as a mismatch
  mirror, not a rescue path.
- Template-instantiation de-dup helpers in `types/template.cpp` and
  direct-emission helpers in `types/base.cpp` preserve rendered final-spelling
  mirrors while refusing to let valid structured-key misses be satisfied by
  legacy rendered entries.

Focused tests in `tests/frontend/frontend_parser_tests.cpp` cover primary and
specialization lookup demotion, NTTP default cache mirror mismatch behavior,
specialization reuse, and direct-emission de-dup mirror healing. No owned code
changes were needed in this packet because the delegated implementation state
already matched the Step 2 contract.

## Suggested Next

Lifecycle review decision: Step 2 is exhausted for the currently identified
pure parser text lookup work. The parser-template mirror family already proves
the structured/TextId-first contract, and the remaining obvious string-keyed
parser family is `struct_tag_def_map`, which is semantic record authority rather
than pure text lookup.

Next bounded implementation packet: execute Step 3's first semantic-record
packet for the parser record-tag bridge.

Suggested owned implementation files:
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/support.cpp`
- `src/frontend/parser/parser_support.hpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/struct.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/impl/expressions.cpp`
- `tests/frontend/frontend_parser_tests.cpp`

Packet goal: inventory each `struct_tag_def_map` read/write path, classify tag
spelling versus record identity, and convert only the first bounded
parser-owned record-tag lookup surface that can use semantic record authority
without requiring downstream HIR/LIR/BIR contract changes. If the bridge cannot
be contained in parser-owned code, stop and record the exact blocker for a
separate open idea instead of widening the packet.

Suggested proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

## Watchouts

Do not treat `struct_tag_def_map` as a pure text-map conversion: struct tags
feed sizeof/alignof, offsetof, template instantiation, and downstream type
surfaces through rendered `TypeSpec::tag` strings. The Step 3 packet must start
from semantic record authority and keep any required `TypeSpec::tag` string as
an explicit bridge until the affected parser paths have a replacement.

Do not delete string overloads wholesale. Public/parser test helpers still use
string inputs as compatibility bridges, and many token-spelling probes are
source spelling or diagnostics rather than lookup authority.

Overfit risk: changing tests by weakening legacy bridge coverage would violate
the plan. The parser template mirror tests intentionally prove both sides:
valid structured misses are not rescued by rendered-name mirrors, while
TextId-less compatibility and final-spelling mirrors remain available.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
