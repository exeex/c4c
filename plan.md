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

Completion check: either identify an omitted legal direct-fact owner or confirm
that no caller has such a fact before signature normalization.

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
