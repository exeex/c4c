# Template Scope Owner And Member Substitution Domain Keys

Status: Closed
Created: 2026-05-08
Closed: 2026-05-09

Parent Ideas:
- `ideas/closed/149_template_instantiation_structured_argument_key.md`
- `ideas/closed/150_nttp_type_binding_domain_key_contract.md`

Related Open Ideas:
- `ideas/open/153_parser_sema_record_tag_compatibility_table_retirement.md`
- `ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md`

## Goal

Replace remaining parser/template string owner and parameter-name substitution
paths with structured template-domain keys shared by parser and Sema.

Parser should preserve template syntax, owner carriers, parameter order,
parameter kind, and spelling `TextId`s. Sema should assign domain identity for
template owners, type parameters, NTTP parameters, member typedefs, and
substitution environments. Parser should not match member or NTTP substitutions
primarily by rendered parameter-name strings.

## Why This Idea Exists

Idea 150 introduced structured parser/HIR type and NTTP binding carriers, but
parser still has residual template substitution surfaces that look stringly:

- `ParserTemplateScopeFrame::owner_struct_tag`
- instantiated member typedef cloning and binding registration
- array-size substitution paths comparing `array_size_expr->name` against
  string NTTP binding names
- member typedef substitution paths that compare legacy typedef tags
- template owner display strings used around instantiated record/member names

These paths sit at a parser/sema boundary. Parser can see partial template
syntax, but Sema is the right place to define final template-domain meaning.

## Responsibility Split

Parser owns template syntax carriers:

- template owner syntax
- parameter names as `TextId`
- parameter index, kind, pack/default flags
- captured default/deferred tokens
- provisional member typedef and instantiation carriers

Sema owns template-domain identity:

- owner template key
- parameter index/kind identity
- type/NTTP binding environments
- member typedef owner and member identity
- non-dependent substitution where enough information is available

HIR may complete dependent substitution later, but it should receive structured
binding carriers rather than rendered parameter-name strings.

## In Scope

- Inventory `ParserTemplateScopeFrame::owner_struct_tag` and all callers.
- Inventory parser template-member substitution paths that compare strings such
  as `array_size_expr->name == npname` or legacy tag strings.
- Define or reuse structured template-owner and parameter-domain keys for the
  covered parser/sema substitution paths.
- Make member typedef and NTTP substitution use owner/index/kind metadata
  before any spelling fallback.
- Preserve rendered template names and mangled names only as display/final
  spelling or compatibility mirrors.
- Add tests with same-spelling template parameters or owners where string
  substitution would bind the wrong value/type.

## Out Of Scope

- Full template instantiation redesign.
- Requiring every dependent expression to resolve in Sema before HIR.
- Removing deferred expression syntax tokens or display debug text.
- Reopening structured argument key work already closed by idea 149.
- Reopening parser deferred NTTP binding-set work already closed by idea 150.

## Acceptance Criteria

- Covered template owner and member substitution paths use structured
  owner/parameter-domain keys before rendered strings.
- `owner_struct_tag`, if retained, is display or compatibility only, with a
  replacement owner key beside it.
- NTTP array-size substitution does not depend on matching `Node::name` against
  string binding names when structured parameter metadata exists.
- Tests cover same-spelling template parameter or owner ambiguity.

## Reviewer Reject Signals

- A slice wraps string parameter names in a new type without adding owner,
  parameter index, and parameter kind.
- Template owner identity is still a rendered struct tag after the slice.
- NTTP substitution succeeds primarily because `std::string(name) == param`.
- Tests protect one named fixture instead of proving domain-key substitution.

## Closure Notes

Closed after implementation of the active runbook for structured owner,
member typedef, and NTTP substitution domain keys. Reviewer report
`review/template_domain_key_closure_review.md` found no blocking findings and
no testcase-overfit. Full-suite validation passed 3023/3023, and regression
guard comparison against `test_baseline.log` reported no new failures with
`--allow-non-decreasing-passed`.
