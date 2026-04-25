# Step 9 Legacy Fallback Readiness Audit

Source plan step: Step 9, `Audit and remediate legacy fallback readiness`.

## Audited Surfaces

- `Module::classify_function_decl_lookup` and `Module::classify_global_decl_lookup`
  preserve precedence: concrete global ID first for globals, then `LinkNameId`,
  then structured key lookup, then legacy rendered lookup.
- `Module::make_decl_ref_lookup_key` requires a valid ref `name_text_id` and a
  complete qualifier metadata vector; incomplete qualifier metadata intentionally
  cannot form a structured key.
- `Module::index_function_decl` and `Module::index_global_decl` dual-write the
  structured mirror only when declaration-side `name_text_id` is available.
- The focused HIR dump fixture
  `tests/cpp/internal/hir_case/module_decl_lookup_structured_mirror_hir.cpp`
  currently emits no legacy-rendered lookup hits. It shows:
  - `global api::value via global-id#...`
  - `function api::bump via structured#...`
  - call-site `api::bump` through `link-name#...`

## Remaining Legacy-Rendered Hits In Focused Proof

The direct frontend HIR module lookup test now enumerates the remaining
fallback-compatible cases without changing resolver behavior:

- Function `legacy_fn`: missing declaration metadata. The declaration has
  invalid `Function::name_text_id`, so the structured mirror is intentionally
  absent and rendered fallback remains required.
- Global `legacy_global`: missing declaration metadata. The declaration has
  invalid `GlobalVar::name_text_id`, so the structured mirror is intentionally
  absent and rendered fallback remains required.
- Function `api::incomplete_qualified_fn`: incomplete qualifier metadata on the
  `DeclRef`. The declaration has a structured key, but the reference has a
  qualifier segment without matching `segment_text_ids`, so the ref cannot form
  a structured lookup key and rendered fallback remains required.
- Global `api::incomplete_qualified_global`: incomplete qualifier metadata on
  the `DeclRef`. Same classification as the function case.

## Remediation Decision

No implementation remediation was applied in this packet. The real lowering
fixture already resolves the namespace-qualified function through structured
lookup, resolves globals through concrete IDs, and resolves the call through
`LinkNameId`. The remaining direct-test legacy hits are compatibility coverage
for missing declaration metadata and incomplete qualifier metadata rather than
safe metadata propagation gaps with existing AST/HIR metadata in hand.

Fallback precedence is unchanged.

