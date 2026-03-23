# Template Function Identity Plan

## Goal

Stop using mangled strings as the primary semantic identity for template
function specialization selection.

Strings should remain useful for:

- diagnostics
- debug dumps
- emitted symbol names
- metadata printing

But they should not be the main mechanism for answering:

- which function template family are we talking about?
- which specialization pattern matched?
- which concrete instantiation does this body belong to?


## Problem

Today template function lowering has two different identity systems mixed
together:

- structured instantiation bindings
  - `TypeBindings`
  - `NttpBindings`
  - `SpecializationKey`
- string-based specialization selection
  - `mangled_name`
  - `mangle_specialization(...)`
  - `specs_[mangled]`

That means the pipeline is only partially structured.

In particular, explicit specialization lookup still depends on:

1. building a mangled name for the concrete instantiation
2. building the same mangled name for the explicit specialization AST node
3. using string equality to decide which specialization body to lower

That works, but it keeps semantic selection tied to printable/codegen names.


## Desired Principle

Template function resolution should follow the same identity model we now use
for template structs:

- family identity = primary function template definition
- pattern identity = a specialization candidate under that primary definition
- concrete instantiation identity = primary definition + concrete bindings

Strings may still describe these identities, but should not define them.


## Target Data Model

Conceptually:

```cpp
struct FunctionTemplateEnv {
  const Node* primary_def = nullptr;
  const std::vector<const Node*>* specialization_patterns = nullptr;
};

struct SelectedFunctionTemplatePattern {
  const Node* primary_def = nullptr;
  const Node* selected_pattern = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SpecializationKey spec_key;
};

struct FunctionTemplateInstanceKey {
  const Node* primary_def = nullptr;
  SpecializationKey spec_key;
};
```

The key idea:

- explicit specialization matching should start from `primary_def`
- selected specialization should be a structured result
- `mangled_name` should be derived output, not the semantic lookup key


## Current State

What is already good:

- template function instantiation discovery already tracks structured bindings
- `SpecializationKey` already exists
- `InstantiationRegistry` already records realized instances separately from
  seed work

What is still weak:

- explicit specialization registry is keyed by mangled string
- specialization body selection uses `inst.mangled_name`
- there is no owner-based `primary_def -> specialization_patterns` API
- there is no structured "selected function specialization pattern" result


## Migration Strategy

### Phase 1. Separate Family Registration From Specialization Registration

Minimum rule:

- primary template functions register as template families
- explicit specializations register under their primary template
- explicit specializations must not behave like independent template origins

This mirrors the template struct guardrail.


### Phase 2. Introduce Owner-Based Function Template Selection

Add helper-level APIs that operate on:

- `const Node* primary_def`
- `const Node* selected_pattern`

instead of:

- only `mangled_name`

Examples:

- build a `FunctionTemplateEnv` from a primary function template
- select specialization pattern from `primary_def + bindings`
- return `SelectedFunctionTemplatePattern`


### Phase 3. Move Explicit Specialization Lookup Off Mangled Strings

Replace:

- `specs_[mangled]`
- `find_specialization(mangled)`

with something conceptually closer to:

- `specializations_[primary_def]`
- structured pattern matching against bindings / `SpecializationKey`

Mangled names may still be stored, but only as derived output.


### Phase 4. Introduce Concrete Function Instance Keys

Use a structured key for semantic identity:

- `primary_def + spec_key`

This key should be suitable for:

- dedup
- specialization selection
- future caching
- future JIT/materialization bookkeeping


### Phase 5. Keep Mangled Names As Derived Data Only

Desired rule:

- semantic identity uses structured keys
- emitted symbol names are derived from the semantic identity
- debug/metadata printing may show `mangled_name`, but lookup should not
  require reparsing or reproducing that string


## Suggested First Implementation Slice

The smallest useful next step is:

1. add owner-based registration for explicit template function
   specializations
2. introduce `FunctionTemplateEnv`
3. introduce `SelectedFunctionTemplatePattern`
4. make lowering choose specialization bodies from structured selection
   first
5. leave mangled-name lookup only as temporary fallback during transition


## Completion Criteria

This plan is done when:

- explicit specialization selection no longer depends primarily on
  `mangled_name`
- function template family identity is the primary template AST node
- specialization pattern selection returns structured results
- `SpecializationKey` participates in semantic identity directly
- mangled names remain only derived/codegen/debug data
