# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Convert Pure Parser Text Lookup

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

Next bounded implementation packet: move to the next pure parser text lookup
family that already has interned text available, or ask plan-owner for a Step 2
substep split if the supervisor wants the remaining pure-text families
enumerated before implementation. Keep `struct_tag_def_map` out of the next
packet unless the supervisor explicitly selects the record-tag bridge family.

## Watchouts

Do not convert `struct_tag_def_map` first: struct tags feed sizeof/alignof,
offsetof, template instantiation, and downstream type surfaces through rendered
`TypeSpec::tag` strings. Treat that as a later semantic-record packet or a
separate bridge blocker if it requires HIR/LIR changes.

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
