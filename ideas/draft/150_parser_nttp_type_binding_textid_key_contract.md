# Parser NTTP Type Binding TextId Key Contract

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
- `ideas/open/146_qualified_name_deferred_carrier_authority.md`

## Goal

Replace string-pair template binding metadata in deferred parser evaluation
APIs with `TextId` plus domain-key binding metadata for type parameters and
non-type template parameters.

String pair forms such as `vector<pair<string, TypeSpec>>` and
`vector<pair<string, long long>>` may remain only as compatibility adapters
while call sites migrate. They should not be the authoritative contract for
binding template parameter names to type or NTTP values.

Deferred binding is expected here. Parser and Sema do not have to fully resolve
every NTTP default, captured expression, or type binding before HIR. They do
have to preserve structured binding carriers so HIR can perform late
substitution without matching by rendered parameter-name strings.

## Why This Idea Exists

Deferred template expression evaluation still passes parameter bindings through
string-keyed APIs and compares token spelling. Current anchors include
`src/frontend/parser/parser.hpp` APIs such as
`eval_deferred_nttp_default`, `eval_deferred_nttp_expr_tokens`, and
`eval_captured_template_arg_expr_tokens`, plus
`src/frontend/parser/impl/types/template.cpp`
`eval_deferred_nttp_expr_tokens`.

Those paths make template parameter identity depend on `std::string` names and
token-spelling comparisons instead of a binding contract. The intended policy
is:

- Lexer interns spelling into `TextId`.
- `TextId` carries no semantics beyond spelling identity.
- Semantic meaning lives in domain tables.
- Template parameter bindings are indexed by parameter-domain keys and
  structured binding metadata, not by rendered strings.

This idea exists to define the boundary between spelling tokens and semantic
template parameter bindings for deferred NTTP and captured template argument
evaluation. HIR may perform late binding substitution, but it must not do
string-based Sema by comparing `std::string` parameter names or token spelling
as the authoritative lookup route.

## Working Responsibility Split

### Parser

Parser owns template parameter and argument syntax carriers.

Parser should:

- preserve parameter-name spelling as `TextId`
- preserve parameter order, kind, and owner template context when known
- preserve type/value binding payloads in structured metadata
- keep legacy string-pair vectors only as adapters while callers migrate

Parser should not:

- make `std::string` parameter names the semantic binding key
- bind NTTP values primarily by `token_spelling(tok) == parameter_name`
- treat parameter-name `TextId` alone as semantic identity across template
  owners

### Sema

Sema owns eager binding interpretation where possible.

Sema should:

- assign parameter-domain identity using owner template key, parameter index,
  parameter kind, and spelling `TextId`
- resolve non-dependent type/value bindings when available
- produce structured deferred binding carriers for dependent defaults and
  captured expressions

### HIR

HIR may perform late binding substitution.

HIR should:

- consume structured binding carriers from Parser/Sema
- resolve deferred NTTP/type bindings under the current instantiation context
- preserve parameter-domain identity through substitution

HIR should not:

- match bindings by `std::string` parameter name
- compare token spelling as the main binding mechanism
- collapse type-parameter and NTTP-value binding into one stringly map

## In Scope

- Inventory deferred evaluation declarations in
  `src/frontend/parser/parser.hpp`, especially `eval_deferred_nttp_default`,
  `eval_deferred_nttp_expr_tokens`, and
  `eval_captured_template_arg_expr_tokens`.
- Inventory the implementation path in
  `src/frontend/parser/impl/types/template.cpp`, especially
  `eval_deferred_nttp_expr_tokens` and any token-spelling comparison used to
  bind parameter values.
- Define binding metadata for type parameters that carries parameter spelling
  `TextId`, owner template key / parameter index / parameter kind where
  available, and the structured type binding payload.
- Define binding metadata for NTTP parameters that carries parameter spelling
  `TextId`, owner template key / parameter index / parameter kind where
  available, and the structured constant/value or deferred expression payload.
- Keep legacy `vector<pair<string, TypeSpec>>` and
  `vector<pair<string, long long>>` forms only as compatibility adapters with
  explicit conversion into the structured binding contract.
- Ensure HIR late substitution paths consume the structured binding contract
  instead of matching string-pair names.
- Add tests or probes where parameter names with equal spelling in different
  template contexts, captured argument expressions, or formatting differences
  would fail under string-pair authority.
- Document any remaining token-spelling comparison as parse-time syntax or
  compatibility behavior, not semantic binding authority.

## Out Of Scope

- Full constant evaluator redesign beyond the binding key contract needed by
  deferred NTTP evaluation.
- Forcing all NTTP/type binding resolution to finish in Sema before HIR.
- Treating `TextId` alone as semantic template parameter identity. `TextId`
  is spelling identity; parameter-domain metadata provides semantic identity.
- Replacing diagnostics, dump output, or source spelling renderers.
- Broad Sema/HIR/backend rewrites except narrow call-site changes required to
  pass structured bindings.
- Weakening existing template tests or converting failures to unsupported.

## Acceptance Criteria

- Deferred NTTP/type binding APIs have a structured binding contract for type
  parameters and NTTP parameters.
- Semantic binding lookup no longer depends on `vector<pair<string,
  TypeSpec>>`, `vector<pair<string, long long>>`, or token-spelling comparison
  as the authoritative path.
- HIR late binding substitution, where required, consumes structured binding
  carriers and does not use rendered parameter-name strings as authority.
- Legacy string-pair APIs, if retained, are compatibility wrappers that convert
  into `TextId` plus domain-key binding metadata.
- Tests or focused probes cover at least one scenario where spelling-only
  parameter matching would choose the wrong binding or fail to distinguish
  template contexts.
- The route preserves the policy that lexer `TextId` is spelling identity
  only and that semantic parameter meaning lives in template/domain binding
  metadata.

## Reviewer Reject Signals

- A slice claims progress by wrapping the same `std::string` pair vectors in a
  new type without changing semantic lookup authority.
- Token spelling comparisons remain the main binding mechanism after the
  slice claims structured binding metadata exists.
- HIR late substitution matches `std::string` parameter names instead of
  parameter-domain binding metadata.
- The implementation treats parameter-name `TextId` alone as semantic
  parameter identity across template contexts.
- Type parameter and NTTP value parameter bindings remain mixed in an
  untyped/stringly map.
- Tests are weakened, marked unsupported, or rewritten around one known
  deferred-expression fixture instead of proving binding identity.
- Compatibility adapters become the only route used by new code without a
  removal condition.
- The exact old failure mode remains behind a new abstraction name: deferred
  evaluation still succeeds primarily because a rendered parameter name string
  matches.
