# Deferred Syntax Text Payload Contract

Status: Closed
Created: 2026-05-08
Closed: 2026-05-09

Parent Ideas:
- `ideas/closed/149_template_instantiation_structured_argument_key.md`
- `ideas/closed/150_nttp_type_binding_domain_key_contract.md`

Related Open Ideas:
- `ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md`
- `ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md`

## Goal

Define and enforce the boundary where parser string payloads are legitimate
deferred syntax text, not semantic identity keys.

Some strings must remain because they are source syntax payloads, diagnostics,
debug text, literal bytes, or final display spelling. This idea ensures those
strings cannot quietly become canonical keys for template arguments, deferred
NTTP expressions, qualified names, record owners, or type identity.

## Why This Idea Exists

Parser cleanup should not become a blanket ban on strings. Helpers such as
`capture_template_arg_expr_text`, template debug refs, deferred NTTP/default
tokens, and display renderers carry source syntax that may need to be reparsed
or evaluated later.

The risk is that these same strings have historically been reused as semantic
keys. Ideas 149 and 150 moved template instantiation and NTTP binding authority
toward structured variants, but the remaining syntax-text payloads need an
explicit contract so future parser/sema/HIR work does not reintroduce string
authority.

## Responsibility Split

Parser owns syntax payload preservation:

- token streams for deferred expressions
- source spelling for diagnostics and debug output
- literal text/bytes where the language requires exact spelling or payload
- display mirrors beside structured carriers

Sema owns semantic interpretation:

- type/value identity
- template parameter binding domains
- overload and qualified-name resolution
- non-dependent constant evaluation where possible

HIR may evaluate deferred payloads later, but must combine them with structured
binding/owner/domain carriers instead of treating payload text as the key.

## In Scope

- Inventory parser strings that represent deferred syntax payloads, including
  `capture_template_arg_expr_text`, template arg debug refs, deferred NTTP
  defaults, and reparse-token helpers.
- Classify each as syntax payload, diagnostic/debug text, display/final
  spelling, compatibility mirror, or semantic key.
- Add API names or comments that make payload status explicit where ambiguity
  exists.
- Ensure payload strings are always paired with structured binding, owner, or
  qualified-name carriers before semantic lookup.
- Add tests where stale debug/syntax text is present but structured metadata
  should determine the result.
- Leave legitimate literal bytes and final output spelling untouched.

## Out Of Scope

- Removing deferred expression tokens.
- Removing diagnostics or debug strings.
- Requiring all dependent expression evaluation to happen in parser or Sema.
- Replacing source literal payloads with TextId when the payload is not a name.
- Reopening LinkNameId or LIR final-symbol spelling work.

## Acceptance Criteria

- Deferred syntax text payloads are documented and named as payloads, not keys.
- Semantic lookup paths touched by this idea require structured domain carriers
  before using deferred text.
- Stale debug/rendered text cannot override structured template, owner,
  qualified-name, or record metadata.
- Remaining text payloads have clear owners and removal/non-removal rationale.

## Closure Summary

The active runbook completed all six steps. Deferred syntax/debug/display text
remains available only as syntax, diagnostic/debug, final spelling, literal
payload, or bounded compatibility mirror, while semantic lookup paths touched
by the plan now prefer structured `TextId`, owner/index, record,
qualified-name, expression, or domain carriers.

Step 6 closure validation passed the selected parser/frontend/HIR/C++ scope
with 1149/1149 tests passing. Close-time regression guard accepted the existing
matching `test_before.log` and `test_after.log` comparison with no new
failures.

Known retained compatibility debt: the foreign-owner type-pack bridge remains
until a structured cross-owner pack binding key maps foreign owner/index
carriers without rendered names.

## Reviewer Reject Signals

- A slice deletes needed syntax text and breaks deferred parsing rather than
  separating payload from identity.
- A payload string is used as a canonical template/type/owner key.
- Tests only prove formatting stability, not semantic authority separation.
- HIR evaluation receives text without the structured carrier needed to
  interpret it safely.
