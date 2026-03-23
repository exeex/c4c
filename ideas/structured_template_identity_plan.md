# Structured Template Identity Plan

## Goal

Stop using strings as the semantic identity for template resolution.

Strings should remain useful for:

- diagnostics
- debug dumps
- mangled symbol names
- human-readable printing

But they should **not** be the primary mechanism for representing:

- template family identity
- specialization pattern identity
- concrete instantiation identity


## Problem

Today the frontend can use string names for several different concepts:

- primary template family name
  - example: `is_signed_helper`
- parser-generated specialization pattern name
  - example: `is_signed_helper__spec_1`
- concrete instantiation name
  - example: `is_signed_helper_T_int___anon_nttp_1_1`
- debug/display name

Those are not the same thing, but the current code can let them flow through
the same string-valued paths.

That creates bugs where:

- a concrete instantiation name is later treated as a new template origin
- a specialization print name is used as a semantic classification key
- primary-family lookup and concrete-instance lookup get mixed together

This is especially dangerous in the lazy template work because the engine
needs a stable identity across multiple fixpoint iterations.


## Core Principle

Template resolution should use structured identity.

That means:

- "which template family is this?" should not be answered by parsing a string
- "which specialization pattern was selected?" should not be encoded only in a
  printable name
- "which concrete instantiation do we mean?" should be a structured key, not a
  guess from a mangled string


## The Three Identities We Need

### 1. Template Family Identity

This is the identity of the primary template itself.

Conceptually:

```cpp
struct TemplateStructFamily {
  const Node* primary_def;
};
```

This answers:

- what is the primary template?
- where do we look for specialization patterns?
- what family do concrete instances belong to?


### 2. Specialization Pattern Identity

This is a candidate pattern under a primary template.

Conceptually:

```cpp
struct TemplateStructPattern {
  const Node* primary_def;
  const Node* pattern_def;  // may equal primary_def
};
```

This answers:

- which specialization pattern matched?
- did we choose the primary or a partial/full specialization?


### 3. Concrete Instantiation Identity

This is the semantic identity of a realized instance.

Conceptually:

```cpp
struct TemplateStructInstanceKey {
  const Node* primary_def;
  SpecializationKey bindings_key;
};
```

Optionally:

```cpp
struct TemplateStructInstance {
  const Node* primary_def;
  const Node* selected_pattern;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  std::string mangled_name; // print/codegen only
};
```

This answers:

- which concrete instantiation is this?
- which selected pattern produced it?
- what bindings define it?


## What Strings Should Still Do

Strings are still useful, but only as derived data:

- pretty-print names
- debug labels
- mangled names used by codegen/runtime lookup
- temporary parser-side transport when no structured identity exists yet

Important rule:

> strings may describe identity, but should not define identity


## Where The Current System Goes Wrong

The current code can let these transitions happen:

1. parser produces a specialization/instantiation node with a generated name
2. HIR stores that node in string-keyed maps
3. later template resolution sees the generated string
4. resolution treats that string as if it were the template family origin

That is how names like:

- `foo__spec_1`
- `foo_T_int`

can accidentally become semantic inputs to later resolution.

That should never happen.


## Target Data Model

We do not need to fully replace every existing string field immediately.

But inside template resolution code, we should move toward:

```cpp
struct TemplateStructEnv {
  const Node* primary_def = nullptr;
  const std::vector<const Node*>* specialization_patterns = nullptr;
};

struct SelectedTemplateStructPattern {
  const Node* primary_def = nullptr;
  const Node* selected_pattern = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
};

struct PendingTemplateTypeWorkItem {
  PendingTemplateTypeKind kind;
  TypeSpec pending_type;
  const Node* owner_primary_def = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SourceSpan span;
  std::string context_name;
};
```

The key part is:

- engine-visible work should carry `const Node* primary_def` or equivalent
  structured identity once available
- not just a name like `tpl_struct_origin = "foo_T_int"`


## Migration Strategy

### Phase 1. Stop Registering Non-Primary Names As Primary Families

Status: done

Minimum rule:

- only primary template definitions should populate the primary-family registry
- specialization patterns should populate a separate specialization registry
- concrete instantiations should not register themselves as template families

Completed:

- primary template struct registration is now guarded so non-primary names do
  not enter the primary-family registry
- specialization patterns are tracked separately from primary template family
  definitions
- dependent specialization patterns are no longer lowered as if they were
  concrete struct instances
- pending template type work now carries `owner_primary_def`

This is the first guardrail, and it is in place.


### Phase 2. Introduce Structured Family/Pattern Handles

Status: in progress

Add helper-level APIs that operate on:

- `const Node* primary_def`
- `const Node* selected_pattern`

instead of:

- `std::string tpl_name`

Examples:

- select specialization from `primary_def`
- build instance key from `primary_def + bindings`
- resolve member typedef against a concrete instance that points back to its
  primary family

Completed:

- template struct lookup now has owner-based parser/HIR helpers that start
  from the primary template node
- HIR template struct selection now uses:
  - `TemplateStructEnv`
  - `SelectedTemplateStructPattern`
- deferred template type work now prefers `owner_primary_def` when available

Remaining:

- convert more `resolve_pending_tpl_struct()` callers to pass explicit
  primary/template env handles instead of relying on `tpl_struct_origin`
- introduce a more explicit concrete instance identity helper
  (`primary_def + bindings`)
- reduce remaining semantic dependence on `template_origin_name` /
  `tpl_struct_origin` outside parser-side transport


### Phase 3. Canonicalize Pending Type Work On Entry To The Engine

Status: started

Even if parser/HIR still carry string transport fields, the engine should try
to canonicalize them into:

- primary template handle
- structured binding key

before doing real resolution work.

Completed:

- `PendingTemplateTypeWorkItem` now carries `owner_primary_def`
- engine dedup for pending template type work now includes the structured
  owner when available

Remaining:

- canonicalize more pending work items at creation time so engine resolution
  does not need to re-discover the primary from string transport
- push `TemplateStructEnv`/selected-pattern data further into deferred
  resolution entry points

This is the point where string transport stops being engine-internal identity.


### Phase 4. Separate Semantic Keys From Mangled Names

Status: not started

Current code often uses a mangled or generated name both as:

- cache key
- semantic key
- printed name

We should split those apart.

Desired rule:

- semantic cache/dedup uses structured keys
- mangled name is derived from the semantic key


### Phase 5. Shrink String-Based Lookup Paths

After the structured APIs exist, start replacing string-based lookups in:

- template struct instantiation
- specialization selection
- member typedef lookup
- dependent base resolution
- template type work dedup

Eventually the remaining string paths should be debug-only or parser-only.


## Interaction With Lazy Instantiation

This plan is separate from, but strongly related to:

- [template_lazy_instantiation_plan.md](/workspaces/c4c/ideas/template_lazy_instantiation_plan.md)

The lazy-instantiation plan answers:

- when is work created?
- who owns retries and fixpoint?
- how does blocked work decompose?

This structured-identity plan answers:

- what semantic object does each work item refer to?
- how do we distinguish primary family / specialization pattern /
  concrete instance?

We need both.

Without lazy instantiation:

- work happens in the wrong place

Without structured identity:

- work may be driven by the engine, but still target the wrong semantic owner


## Minimal Success Criteria

This refactor is worthwhile when all of the following are true:

- generated specialization names are never treated as primary template names
- concrete instantiation names are never reused as template-family origins
- engine work items can identify the intended primary template without parsing
  a string
- specialization selection and instantiation caching use structured semantic
  keys rather than printable names


## Short Version

The desired model is:

- primary template = structured identity
- specialization pattern = structured identity
- concrete instantiation = structured identity
- strings = print/mangle/debug only
