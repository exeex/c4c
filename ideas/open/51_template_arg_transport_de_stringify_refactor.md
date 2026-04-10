# Template Arg Transport De-Stringify Refactor

Status: Open
Last Updated: 2026-04-10

## Goal

Refactor template argument transport so parser, deferred NTTP evaluation, and
HIR materialization stop depending on ad hoc string-carried type spellings as
their primary semantic representation.

The immediate objective is not a full template-system rewrite. It is to replace
the most failure-prone "serialize `TypeSpec` to text, then later re-lex /
re-decode it" paths with structured template-arg data that can survive across
deferred template work without losing enum / struct / typedef / cv-ref
information.

## Why This Idea Exists

Recent EASTL and trait follow-up work exposed the same structural weakness more
than once:

- template/deferred flows often transport type arguments through strings such as
  `enum_ns::Color`, `struct_Foo`, or `const_uint_ref`
- later phases then reconstruct `TypeSpec` by splitting and decoding those
  strings with local helper logic
- the decode rules are distributed across parser and HIR code instead of living
  behind one structured boundary

That shape is convenient for logging and quick debugging, but it is expensive
to maintain and easy to mis-handle. The result is visible in recently observed
failure modes:

- deferred builtin-trait folding for qualified enum types needed new string
  decoding glue just to preserve `__is_enum(T)` through inherited
  `integral_constant<bool, ...>` bases
- HIR deferred member-type resolution was still reachable with only
  `tpl_struct_origin` and no concrete `tag`, which made fallback code build
  `std::string(nullptr)` and crash
- EASTL progression continues to expose places where semantic type information
  is lost and later reconstructed heuristically

These are symptoms of the same design pressure: structured type/template data is
repeatedly flattened into text too early.

## Main Objective

Introduce a structured template-argument transport path so the compiler can
carry semantic type information directly across deferred template work, while
keeping compatibility with existing string-keyed identity/mangling surfaces
during migration.

Success means:

- deferred template evaluation can consult structured type/value args instead of
  reparsing text in the common case
- enum / struct / typedef / cv-ref information survives transport without ad hoc
  prefix decoding at each use site
- string spellings remain available for diagnostics, cache keys, and temporary
  compatibility, but are no longer the primary semantic payload
- parser/HIR template helpers stop needing scattered "if text starts with
  `enum_` / `struct_` / `union_`" recovery logic

## Current Pressure Points

The highest-value string-carried transport fields and flows to audit first are:

- `TypeSpec::{tpl_struct_origin, tpl_struct_arg_refs, deferred_member_type_name}`
- parser substitution paths that build `new_arg_parts` and later rebuild
  `ParsedTemplateArg`
- deferred NTTP expression evaluation in
  `src/frontend/parser/parser_types_template.cpp`
- HIR template arg materialization and deferred member-type lookup in
  `src/frontend/hir/hir_templates.cpp`

Especially risky patterns:

- `append_type_mangled_suffix(...)` followed by later decode back into `TypeSpec`
- `$expr:` deferred NTTP text substitution that rewrites type params into
  mangled type spellings
- fallback decoding that guesses meaning from text prefixes rather than from an
  explicit arg kind / structured type payload

## Proposed Direction

### 1. Add Structured Template Arg Transport

Introduce an internal transport structure along these lines:

- `TemplateArgRef`
- `TemplateArgRefList`

Expected payload:

- whether the arg is a type arg or NTTP value arg
- a structured `TypeSpec` when the arg is a type
- the integer value when the arg is an NTTP
- optional original/debug text for diagnostics only

The minimum viable destination is that `TypeSpec` can carry parsed template args
alongside the current `tpl_struct_arg_refs` string field instead of only the
string form.

Illustrative direction for the transport shape:

```cpp
enum class TemplateArgKind : uint8_t {
  Type,
  Value,
};

struct TemplateArgRef {
  TemplateArgKind kind = TemplateArgKind::Type;
  TypeSpec type{};
  long long value = 0;

  // Optional: preserve the original/debug spelling for diagnostics only.
  const char* debug_text = nullptr;
};

struct TemplateArgRefList {
  TemplateArgRef* data = nullptr;
  int size = 0;
};

struct TypeSpec {
  TypeBase base;
  const char* tag;

  const char* tpl_struct_origin;

  // Legacy compatibility/debug transport. Not the long-term semantic source.
  const char* tpl_struct_arg_refs;

  // New structured transport.
  TemplateArgRefList tpl_struct_args;

  const char* deferred_member_type_name;
};
```

The key point is that enum/struct/typedef/cv-ref information should remain in
`TypeSpec` and `TemplateArgRef`, while any mangled spelling becomes a derived
debug/identity view rather than the primary semantic payload. In other words:

- the data structure should carry `TB_ENUM`, `TB_STRUCT`, `TB_TYPEDEF`, etc.
- helper printers may still render `enum_ns::Color` or `const_uint_ref`
- downstream semantic code should branch on enums/fields, not on string prefixes

### 2. Keep String Compatibility During Migration

Do not break current identity/mangling or deferred-owner plumbing all at once.

Migration should preserve compatibility by:

1. adding structured arg storage alongside existing string refs
2. switching readers to prefer structured args when available
3. leaving string refs available for cache keys / diagnostics / untouched
   compatibility paths
4. deleting or narrowing string-based semantic decode only after all active
   readers have migrated

### 3. Centralize Any Remaining Encode/Decode Boundary

If string transport must still exist temporarily, it should be owned by a small
number of encode/decode helpers rather than open-coded throughout parser and
HIR. There should be one obvious place to audit when a new type shape appears.

### 4. Reduce Semantic Work Done On Text

Deferred evaluators should not have to infer semantic facts by reparsing text.

Specific targets:

- builtin trait folding should consume structured `TypeSpec`
- template-base arg reconstruction should consume typed args, not
  `new_arg_parts` strings where practical
- deferred member-type resolution should avoid null/empty fallback assumptions
  that exist only because owner/type identity is partially text-based

## Primary Scope

- `src/frontend/parser/ast.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- `src/frontend/parser/types_helpers.hpp`
- `src/frontend/hir/hir_templates.cpp`
- any closely related compile-time / template transport helper files touched by
  the migration

## Non-Goals

- no full replacement of existing mangled-name identity schemes in one slice
- no broad rewrite of template specialization selection logic
- no deprecation of all string diagnostics or debug spellings
- no unrelated EASTL-specific workaround work hidden inside this refactor
- no codegen/LIR refactor beyond the template/type transport issues directly
  exposed here

## Required Migration Strategy

This refactor should be staged, because template transport is shared across
parser, HIR, and deferred compile-time paths:

1. identify the canonical structured arg container and add it alongside current
   string transport
2. populate structured args at the earliest practical construction sites
3. switch deferred NTTP and template-base reconstruction helpers to prefer
   structured args
4. switch HIR materialization and deferred member-type logic to structured args
   first
5. prune redundant text decode paths only after behavior remains stable

## Validation

At minimum, validation should include:

- focused regressions for deferred builtin traits on qualified user-defined
  types
- focused regressions for inherited `integral_constant<bool, trait(T)>` bases
- focused regressions for alias/member-typedef-backed trait paths already
  covered by EASTL follow-up work
- `eastl_type_traits_simple_workflow`
- full `ctest --test-dir build -j8 --output-on-failure` before closing the idea

## Guardrails

- prefer structural semantic transport over adding more prefix-based string
  decoding special cases
- keep any temporary compatibility string fields explicitly documented
- if a new typed transport field is added, document which readers still rely on
  the legacy string transport
- do not silently expand into a full canonical-type-system rewrite; split a new
  idea if the migration grows beyond template arg transport

## Expected Payoff

This refactor should reduce repeated "fix one more string decode site" churn in
template-heavy work, especially EASTL and trait-family follow-ups. It should
also make future template debugging clearer by separating:

- structured semantic transport used by the compiler
- textual spellings used for diagnostics, keys, and debugging

That boundary is currently too blurry, and recent regressions show the cost.
