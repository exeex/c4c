# HIR Function-Signature Definition-Provenance Architecture Blocker Runbook

Status: Active
Source Idea: ideas/open/851_hir_function_signature_definition_provenance_architecture_blocker.md
Supersedes: 849 Step 2 after its intentionally concluded no-change route

## Purpose

Resolve the architecture decision that 849 and 848 cannot resolve within their
bounded forwarding and occurrence-population scopes.

## Core Rule

Canonical aggregate authority must originate as a direct HIR definition-backed
fact. Never recover it from parser, `TypeSpec`, record, owner, tag, text,
`Node*`, or LIR state.

## Ordered Steps

### Step 1 - Confirm the production provenance timeline

Goal: make the negative premise durable and check whether any earlier legal HIR
semantic-construction owner was omitted from the 849 trace.

Actions:

- Inspect only production function-signature construction and HIR aggregate
  definition registration timing.
- Record the complete caller and registration evidence in the source decision.
- Do not change C++ or tests.

Completion check: completed. The complete production caller set is
`hir_build.cpp:968,970,983,988,990,1064,1070` and
`impl/stmt/decl.cpp:103,105`; `lower_function` receives only its function
`Node*`, optional name override, and template/NTTP bindings. Aggregate
definitions receive module refs at `hir_types.cpp:3581` and materialized
template instances at `impl/templates/struct_instantiation.cpp:566`, before
free functions are lowered, but that module state is not issued as a direct
fact to signature construction. The separate later `lower_struct_method`
route does not produce one for free functions. No omitted legal owner exists.

### Step 2 - Decide the legal successor or no-feasible-route conclusion

Goal: make the current architecture's outcome executable rather than leaving
848 parked.

Actions:

- If Step 1 identifies a legal direct-fact owner, create a separately scoped
  implementation successor with exact boundaries and return conditions.
- Otherwise record a no-feasible-route conclusion and route 848 to its own
  bounded no-change disposition; retain 838 as blocked.
- Do not implement, recover identity, or adjust expectations.

Completion check: a new agent can tell exactly whether 848 may resume and, if
so, which approved successor owns the required implementation.

Step 1 decision route: take the otherwise branch. A module lookup keyed by
normalized signature state would be forbidden recovery, so the pre-existing
registrations cannot be used as a successor premise. Step 2 must conclude the
current function-signature route deliberately no-change; it must not create an
implementation successor or claim 838 unblocked.
