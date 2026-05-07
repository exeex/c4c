# Parser Template Instantiation Structured Argument Key

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
- `ideas/open/146_qualified_name_deferred_carrier_authority.md`

## Goal

Replace rendered-string template argument keys in parser template
instantiation metadata with structured type and value argument keys.

`TemplateInstantiationKey::Argument` should represent type arguments and
value arguments through domain-specific structured variants. No semantic key
for a template instantiation should be a canonical rendered spelling, debug
string, or reparsed text fragment.

Template instantiation identity may include either resolved domain identity or
deferred structured argument identity. The key point is not that Parser or Sema
must resolve every template argument immediately; the key point is that
deferred arguments must not be stored as canonical rendered/debug strings.

Parser should preserve template argument syntax as structured carriers. Sema
should resolve type/value arguments eagerly where possible and produce
structured deferred carriers when dependent context prevents a final answer.
HIR may complete late template argument resolution during instantiation, but it
must consume those structured carriers rather than reparsing display text.

## Why This Idea Exists

Template instantiation metadata still contains a string-shaped authority point:
`TemplateInstantiationKey::Argument` in
`src/frontend/parser/impl/parser_state.hpp` uses a `std::string canonical_key`,
and helper construction around
`src/frontend/parser/impl/types/types_helpers.hpp`
`make_template_instantiation_argument_key` can turn type or value arguments
into canonical rendered/debug strings.

That undermines the parser string-authority cleanup because template identity
can still depend on text formatting choices rather than domain identity. The
intended policy is:

- Lexer interns spelling into `TextId`.
- `TextId` carries no semantics beyond spelling identity.
- Semantic meaning lives in domain tables.
- Compound names and template arguments are indexed by `TextId` sequences,
  type-domain keys, value-domain keys, and template-domain keys, not by
  rendered strings.

This idea exists to give template instantiation arguments a structured
contract that can distinguish type arguments from value arguments without
depending on canonical display text, while still allowing dependent arguments
to be completed later by HIR/template instantiation.

## Working Responsibility Split

### Parser

Parser owns template-argument syntax carriers.

Parser should:

- preserve type arguments as structured `TypeSpec` / type-reference carriers
- preserve value arguments as structured token/expression/NTTP carriers
- preserve template argument order and kind without canonicalizing to a string
- keep rendered/debug spelling only for diagnostics and dumps

Parser should not:

- encode semantic template argument identity as `std::string canonical_key`
- collapse type and value arguments into one debug-text comparison path
- treat a raw argument spelling `TextId` as the full semantic argument key

### Sema

Sema owns eager template argument interpretation where possible.

Sema should:

- turn non-dependent type arguments into type-domain identity
- turn non-dependent value arguments into value/constant/NTTP-domain identity
- preserve dependent arguments as structured deferred carriers
- attach owner template, parameter index/kind, and substitution context where
  needed

### HIR

HIR may complete late template argument resolution for deferred/dependent
cases.

HIR should:

- consume resolved Sema identities when available
- consume structured deferred type/value argument carriers otherwise
- produce HIR/module-domain template instantiation identity

HIR should not:

- decide template argument equality by canonical rendered/debug strings
- split or reparse formatted type/value text
- use parser compatibility strings as late template semantic authority

## In Scope

- Inventory `TemplateInstantiationKey::Argument` in
  `src/frontend/parser/impl/parser_state.hpp` and all construction/comparison
  sites that use `canonical_key`.
- Inventory `make_template_instantiation_argument_key` and related helpers in
  `src/frontend/parser/impl/types/types_helpers.hpp`.
- Define a structured argument representation that distinguishes type
  arguments from value arguments, preferably as an explicit variant with
  domain-specific payloads.
- Ensure type arguments are keyed by structured type identity or a provisional
  type-domain/deferred type key, not by rendered `TypeSpec` text.
- Ensure value arguments are keyed by structured expression / constant /
  NTTP-domain/deferred value metadata, not by debug spelling.
- Ensure HIR late instantiation can consume deferred structured argument keys
  without falling back to rendered canonical strings.
- Preserve rendered forms for diagnostics, dumps, and debug display only.
- Add tests or focused probes where two arguments with equivalent or ambiguous
  rendered text would otherwise collide, or where formatting differences would
  incorrectly split one semantic argument.
- Mark any remaining string key as a compatibility mirror with clear ownership
  and removal criteria.

## Out Of Scope

- Full template instantiation semantics, overload resolution, or constant
  evaluation redesign beyond the argument-key contract.
- Forcing all template argument resolution to finish in Sema before HIR.
- Treating raw `TextId` spelling as the semantic template argument key.
- Removing diagnostic and debug renderings of template arguments.
- Broad Sema/HIR/backend rewrites except narrow call-site changes needed to
  consume the structured argument key.
- Rewriting tests or expectations to avoid fixing template argument identity.

## Acceptance Criteria

- `TemplateInstantiationKey::Argument` no longer uses `std::string
  canonical_key` as semantic identity for template arguments.
- Type arguments and value arguments have explicit structured key variants or
  an equivalent domain-specific representation.
- `make_template_instantiation_argument_key` and its callers no longer derive
  semantic identity primarily from rendered or debug strings.
- Deferred/dependent arguments remain structured and can be completed later by
  HIR/template instantiation without string reparsing.
- Rendered argument spelling, if retained, is clearly display-only or
  compatibility-only.
- Focused tests or probes prove that semantic template argument identity does
  not change because of formatting, display spelling, or ambiguous rendered
  text.
- The route preserves the policy that `TextId` is spelling identity only and
  that semantic template argument meaning lives in domain tables or structured
  domain keys.

## Reviewer Reject Signals

- A slice claims progress by renaming `canonical_key` while preserving a
  string as the semantic template argument identity.
- Type and value arguments remain collapsed into one rendered/debug string
  comparison path.
- HIR late instantiation compares template arguments through canonical strings
  instead of structured deferred carriers.
- The implementation treats `TextId` spelling alone as a complete semantic
  argument key without type/value domain metadata.
- Tests are weakened, marked unsupported, or rewritten so that string-key
  collisions are no longer exercised.
- The route adds named-case formatting shortcuts for one template fixture
  instead of introducing structured argument keys.
- Broad template or backend rewrites obscure the fact that
  `TemplateInstantiationKey::Argument` still depends on canonical text.
- The exact old failure mode remains behind a new abstraction name: template
  instantiation identity still succeeds primarily because two rendered strings
  match.
