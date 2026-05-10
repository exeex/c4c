Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Canonical String Authority

# Current Packet

## Just Finished

Step 1 inventory completed for `src/frontend/sema/canonical_symbol.hpp` and
`src/frontend/sema/canonical_symbol.cpp` with no implementation changes.

Inventoried string fields and classifications:
- `CanonicalScopeSegment::name` (`canonical_symbol.hpp`): semantic authority
  when present in `CanonicalScope`, because `mangle_name` treats namespace and
  record segment names as scoped symbol identity text; currently no structured
  owner/scope key is carried beside it. Also ABI output input when rendered by
  `mangle_name`.
- `CanonicalTemplateParam::name` (`canonical_symbol.hpp`): semantic authority
  for template substitution because `substitute_template_args` builds bindings
  from this spelling. Rendering output in `format_canonical_result`.
- `CanonicalType::user_spelling` (`canonical_symbol.hpp`): semantic authority
  for nominal type equality and template-parameter substitution when the type
  is a `TypedefName`; compatibility bridge/display text for leaf nominal type
  spelling; ABI output input in `mangle_type_impl`; diagnostic/debug text in
  `format_canonical_type`; TypeSpec compatibility bridge in
  `typespec_from_canonical`.
- `CanonicalSymbol::source_name` (`canonical_symbol.hpp`): semantic authority
  for `CanonicalIdentity::name` through `identity_of`; ABI output input in
  `mangle_symbol`/`mangle_name`; diagnostic/debug text in
  `format_canonical_result`; fallback text for anonymous function/global/type
  placeholders in `collect_top_level_symbol`.
- `CanonicalIdentity::name` (`canonical_symbol.hpp`): semantic authority for
  symbol-table lookup equality and hashing through `CanonicalIdentity::operator==`,
  `CanonicalIdentityHash`, and `CanonicalSymbolTable::by_identity`.

String-keyed lookups:
- `std::unordered_map<std::string, CanonicalTemplateArg> bindings` in
  `substitute_template_args`/`substitute_template_args_impl`
  (`canonical_symbol.cpp`): semantic authority; keys are raw
  `CanonicalTemplateParam::name` spellings, and lookup uses
  `CanonicalType::user_spelling` for `TypedefName` leaves.
- `std::unordered_map<CanonicalIdentity, CanonicalSymbol, CanonicalIdentityHash>
  by_identity` (`canonical_symbol.hpp/.cpp`): semantic authority; the effective
  string key is `CanonicalIdentity::name`, populated from
  `CanonicalSymbol::source_name`.

String equality comparisons and hash inputs:
- `std::string_view(node->linkage_spec) == "C"` (`canonical_symbol.cpp`):
  semantic authority for linkage selection; structured enough for the current
  parser contract and outside the canonical identity migration target.
- `type.user_spelling.empty()` gates in substitution, formatting, mangling, and
  TypeSpec conversion (`canonical_symbol.cpp`): semantic gate in substitution;
  rendering/ABI/compatibility gate elsewhere.
- `a.user_spelling == b.user_spelling` in `types_equal`
  (`canonical_symbol.cpp`): semantic authority for struct/union/enum/typedef
  nominal equality.
- `name != o.name` in `CanonicalIdentity::operator==`
  (`canonical_symbol.cpp`): semantic authority for symbol identity.
- `std::hash<std::string>{}(id.name)` in `CanonicalIdentityHash`
  (`canonical_symbol.cpp`): semantic authority for symbol-table hash bucket
  selection.
- `seg.name.empty()`, `ct.user_spelling.size()`, `ct.user_spelling.empty()`,
  `name.size()`, and direct appends to `out` in mangling helpers
  (`canonical_symbol.cpp`): ABI output only.

Retained rendered-string fallbacks:
- Owner: `canonical_leaf_display_spelling`/`canonicalize_base_type`.
  Limitation: picks `record_def` display name or legacy `TypeSpec::tag`, not a
  nominal identity key. Removal condition: covered record/enum/typedef paths
  carry structured nominal identity beside display spelling.
- Owner: `collect_top_level_symbol`.
  Limitation: `source_name` and `user_spelling` are populated from rendered
  `Node::name` or anonymous placeholder text. Removal condition: symbols and
  nominal leaves carry structured declaration/owner/domain identity and keep
  rendered names only for display and ABI output.
- Owner: `substitute_template_args`/`substitute_template_args_impl`.
  Limitation: same-spelled template parameters collide because bindings are
  keyed by raw spelling and looked up through `TypedefName::user_spelling`.
  Removal condition: template params and template-param type leaves carry an
  owner-aware parameter key, with spelling fallback restricted to missing
  metadata paths.
- Owner: `types_equal`.
  Limitation: same-spelled nominal leaves compare equal across distinct
  structured domains. Removal condition: nominal and template-parameter leaves
  compare structured identity when both sides have it, using spelling only as
  an explicit no-metadata compatibility bridge.
- Owner: `identity_of`/`CanonicalIdentityHash`/`CanonicalSymbolTable`.
  Limitation: symbol identity and hash use rendered `source_name` without
  structured scope/domain keying. Removal condition: canonical identity carries
  structured symbol identity and hashes it for covered metadata paths.
- Owner: `mangle_type_impl`, `mangle_name`, `mangle_symbol`, and formatting
  helpers. Limitation: these must keep rendered strings for ABI/debug output
  but should not become semantic lookup authority. Removal condition: none for
  rendering itself; only remove semantic dependence on these strings.

## Suggested Next

Start Step 2 by adding structured identity carriers beside the classified
display spellings in `CanonicalScopeSegment`, `CanonicalTemplateParam`,
`CanonicalType`, `CanonicalSymbol`, and `CanonicalIdentity`, keeping no-metadata
fallbacks explicit.

## Watchouts

- Do not treat rendered spelling, ABI mangling text, or debug formatting as
  semantic authority for covered metadata paths.
- Do not replace a string key with a raw `TextId` when owner, scope, template,
  or nominal-domain context is required.
- Document any retained rendered-string fallback with owner, limitation, and
  removal condition.

## Proof

No build or test proof was run; the delegated proof was inventory-only with no
implementation behavior changes, so no `test_after.log` was produced.
