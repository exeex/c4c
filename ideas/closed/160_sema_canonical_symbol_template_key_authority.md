# Sema Canonical Symbol Template Key Authority

Status: Closed
Created: 2026-05-10
Closed: 2026-05-10

Parent Ideas:
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/153_parser_residual_text_payload_audit.md`

## Goal

Make `src/frontend/sema/canonical_symbol.hpp` and
`src/frontend/sema/canonical_symbol.cpp` stop treating rendered spelling as
ordinary semantic authority for canonical identity, nominal type identity, and
template parameter substitution.

Canonical symbol code may continue producing `std::string` for diagnostics,
debug formatting, ABI mangling text, and compatibility display. It should not
use those rendered strings as the primary key when parser/sema metadata already
contains `TextId`, `QualifiedNameKey`, template owner identity, or nominal
record/type identity.

## Why This Idea Exists

Idea 159 cleaned the consteval evaluator and explicitly left full
`canonical_symbol.cpp` cleanup out of scope. The next sema layer still has
string-first identity paths:

- `CanonicalScopeSegment::name` stores scope spelling without a domain key.
- `CanonicalTemplateParam::name` is used as the template binding key.
- `CanonicalType::user_spelling` is used for nominal type equality,
  template-parameter substitution, and type mangling input.
- `CanonicalSymbol::source_name` and `CanonicalIdentity::name` are used as
  symbol identity keys.
- `substitute_template_args_impl` receives
  `unordered_map<std::string, CanonicalTemplateArg>` and looks up
  `type.user_spelling`.
- `types_equal` compares struct/union/enum/typedef names by rendered spelling.
- `CanonicalIdentityHash` hashes rendered symbol names.

Those strings are appropriate renderings, but they are too weak as semantic
identity. Two declarations can share spelling while belonging to different
owners, scopes, template parameter lists, or nominal domains. Conversely,
qualified/namespaced spelling should be represented by structured path keys,
not by parsing a rendered string again.

## Responsibility Split

Parser owns syntax carriers:

- token spelling interned as `TextId`
- qualifier segment `TextId` sequences
- `QualifiedNameKey` for structured names
- template parameter spelling plus owner template/function identity
- source spelling retained for diagnostics and round-trip display

Sema owns canonical semantic identity:

- canonical symbol names should carry domain keys beside display spelling
- nominal type leaves should carry record/enum/typedef identity beside display
  spelling
- template parameter substitution should bind by parameter identity, not by
  rendered name alone
- complete structured metadata misses should not reopen rendered-spelling
  fallback for covered paths

HIR/backend may still consume rendered names for final emission, ABI text, and
debug output, but canonical sema identity must already be domain-keyed before
that handoff.

## In Scope

- Inventory every `std::string` field and string lookup in
  `canonical_symbol.hpp/.cpp`, classifying each as semantic authority,
  rendering output, diagnostic/debug text, ABI output, or compatibility bridge.
- Add structured identity fields where canonical sema needs them, such as
  `TextId` for unqualified names, `QualifiedNameKey` for scoped/qualified
  names, and explicit owner-aware template parameter keys.
- Replace template substitution binding from
  `unordered_map<std::string, CanonicalTemplateArg>` to a structured
  template-parameter key when metadata is complete.
- Make nominal type equality prefer structured nominal identity for
  struct/union/enum/typedef/template-parameter leaves before comparing rendered
  `user_spelling`.
- Make `CanonicalIdentity` and `CanonicalIdentityHash` use structured symbol
  identity where available, with rendered `name` retained as display and
  no-metadata compatibility.
- Keep ABI mangling and debug formatting as string renderers, but feed them
  from canonical identity/display fields rather than letting mangled text
  become semantic identity.
- Add focused tests where same-spelled names in different namespaces, records,
  or template owners must not collide in canonical symbol identity or template
  substitution.
- Document retained rendered-string fallbacks with owner, limitation, and
  removal condition.

## Out Of Scope

- Full HIR `TypeBindings` / `NttpBindings` storage migration.
- Backend `LinkNameId` and final symbol emission table migration.
- Rewriting the Itanium ABI mangler beyond separating rendering from identity.
- Removing diagnostic strings, source spelling strings, or debug formatting.
- Reworking parser name construction that was already covered by ideas
  153-157 unless canonical symbol needs one narrow metadata carrier.
- Weakening tests, changing expected behavior only through rendered-name
  updates, or adding named-case shortcuts.

## Candidate Anchors

- `CanonicalScopeSegment`, `CanonicalTemplateParam`, `CanonicalType`,
  `CanonicalSymbol`, and `CanonicalIdentity` in
  `src/frontend/sema/canonical_symbol.hpp`.
- `canonical_leaf_display_spelling`, `canonicalize_base_type`, and
  `collect_top_level_symbol` in `src/frontend/sema/canonical_symbol.cpp`.
- `substitute_template_args_impl` and `substitute_template_args`, currently
  keyed by `std::string`.
- `types_equal` nominal-name comparison for struct/union/enum/typedef leaves.
- `mangle_type_impl`, `mangle_name`, and `mangle_symbol` as rendering-only
  consumers.
- `CanonicalIdentityHash`, `identity_of`, and `CanonicalSymbolTable` lookup.

## Acceptance Criteria

- `canonical_symbol.hpp/.cpp` string fields and string lookups are inventoried
  and classified.
- Covered canonical symbol/type/template identity paths use structured keys or
  `TextId` metadata before rendered spelling.
- Template substitution does not bind template parameters by raw
  `std::string` name when owner-aware metadata is complete.
- Nominal type equality and canonical symbol table identity do not collide
  same-spelled declarations from different structured domains.
- ABI mangling and formatting remain string-producing output paths, not
  semantic lookup authority.
- Retained rendered-string fallbacks are explicit compatibility/no-metadata
  bridges with removal conditions.
- Focused tests prove stale or same-spelled rendered names cannot override
  structured canonical identity.

## Reviewer Reject Signals

- A slice claims canonical identity is structured while equality or hashing
  still compares only rendered spelling for covered metadata.
- Template substitution still builds a string-keyed parameter binding table as
  the normal path.
- Mangled ABI strings become the new semantic lookup key.
- A raw `TextId` is treated as complete cross-domain identity without owner,
  scope, or nominal-domain context where those are needed.
- Tests only update debug/mangle text without proving identity separation.

## Closure Note

Closed after the canonical close-scope regression guard passed for command-matched
`test_before.log` and `test_after.log` using:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

The guard result was accepted with
`--allow-non-decreasing-passed`: before and after both passed 3/3 with no new
failures. `test_baseline.new.log` was not accepted as a full-suite baseline
because its known failures are unrelated to this canonical-symbol source idea.
