# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Demote struct_tag_def_map To Compatibility Mirror

## Just Finished

Step 5 first packet inventoried all remaining
`DefinitionState::struct_tag_def_map` references and classified them after the
typed `TypeSpec::record_def` propagation work.

Compatibility/final-spelling mirror writers and cache hooks:
- `parser_state.hpp` keeps the rendered-spelling map as parser state.
- `support.cpp::register_struct_definition_for_testing()` is a test hook for
  compatibility/stale-map coverage.
- `types/struct.cpp::register_record_definition()` writes source and canonical
  spelling keys for declared records.
- `types/template.cpp::ensure_template_struct_instantiated_from_args()` writes
  explicit specialization/injected instantiation keys and uses `count()` for
  rendered-instantiation cache/dedup behavior.
- `types/base.cpp` direct template emission writes the mangled instance key as
  the retained final-spelling compatibility mirror.
- parser tests intentionally mutate or assert the map as stale/fallback mirror
  coverage, not as the desired semantic authority.

Typed-identity fallback readers that already prefer `record_def` when a
`TypeSpec` is available:
- `support.cpp::resolve_record_type_spec()` is the central typed-first helper;
  layout, `alignof`, `sizeof`, and `offsetof` use its map lookup only for
  tag-only compatibility.
- `declarations.cpp` incomplete-object checks use `resolve_record_type_spec()`
  and keep `eval_const_int()` map arguments for constant-expression layout
  fallback.
- `expressions.cpp` `__builtin_offsetof` folding passes the map into
  `eval_const_int()` so `NK_OFFSETOF` can use typed identity first and
  compatibility spelling second.
- `types/base.cpp` member typedef owner lookup, template base reconstruction,
  deferred member resolution, and template suffix handling either use
  `resolve_record_type_spec()` first or use map lookup only after an
  instantiation/helper boundary where a rendered key is the compatibility
  bridge.
- `types/declarator.cpp` dependent typename owner and nested owner traversal
  use `resolve_record_type_spec()` first; remaining map finds are fallback for
  tag-only or qualified-name compatibility.
- `types/struct.cpp` bitfield width evaluation passes the map to
  `eval_const_int()` for constant-expression compatibility.

Still-semantic authority / Step 5 blocker:
- `types/template.cpp` template static-member lookup obtains `sdef` solely via
  `struct_tag_def_map.find(ref_mangled)` after
  `ensure_template_struct_instantiated_from_args()`, and its recursive base
  search follows `base_ts.tag` through the map instead of consulting
  `base_ts.record_def` first. This should be converted before the map can be
  described as only a compatibility mirror.

## Suggested Next

Convert the `types/template.cpp` template static-member lookup blocker: ask
`ensure_template_struct_instantiated_from_args()` for a resolved `TypeSpec`,
walk `record_def` when present, and keep rendered-map lookup only as fallback.
Also make the recursive base walk prefer `resolve_record_type_spec(base_ts,
...)` before `base_ts.tag` fallback. This next packet should include focused
coverage where a stale rendered base tag cannot redirect template static
member lookup.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys during Step 5. Preserve `TypeSpec::tag` as spelling, diagnostics, emitted
text, and compatibility payload.

Do not infer semantic record identity from an existing rendered map entry when
a typed `record_def` producer should be carrying the identity directly. Stale
map-entry tests are the guardrail for this step.

The inventory packet made no behavior changes and deliberately did not rename
or document retained map sites while the template static-member reader remains
primary semantic authority.

## Proof

Delegated proof for this inventory packet:

`git diff --check -- todo.md src/frontend/parser/impl/parser_state.hpp src/frontend/parser/impl/support.cpp src/frontend/parser/impl/types/base.cpp src/frontend/parser/impl/types/template.cpp src/frontend/parser/impl/types/declarator.cpp src/frontend/parser/impl/types/struct.cpp src/frontend/parser/impl/declarations.cpp src/frontend/parser/impl/expressions.cpp tests/frontend/frontend_parser_tests.cpp`

No `test_after.log` update was required because this packet changed only
`todo.md` inventory notes.
