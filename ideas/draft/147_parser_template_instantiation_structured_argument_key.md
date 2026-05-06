# Parser Template Instantiation Structured Argument Key

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`

## Goal

Replace rendered-string template argument keys in parser template
instantiation metadata with structured type and value argument keys.

`TemplateInstantiationKey::Argument` should represent type arguments and
value arguments through domain-specific structured variants. No semantic key
for a template instantiation should be a canonical rendered spelling, debug
string, or reparsed text fragment.

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
depending on canonical display text.

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
  type-domain key, not by rendered `TypeSpec` text.
- Ensure value arguments are keyed by structured expression / constant /
  NTTP-domain metadata, not by debug spelling.
- Preserve rendered forms for diagnostics, dumps, and debug display only.
- Add tests or focused probes where two arguments with equivalent or ambiguous
  rendered text would otherwise collide, or where formatting differences would
  incorrectly split one semantic argument.
- Mark any remaining string key as a compatibility mirror with clear ownership
  and removal criteria.

## Out Of Scope

- Full template instantiation semantics, overload resolution, or constant
  evaluation redesign beyond the argument-key contract.
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
