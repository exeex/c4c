# Current Packet

Status: Active
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Add Sema Structured Key Helpers

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1. No
implementation packet has run yet.

## Suggested Next

Delegate Step 1 to an executor with a supervisor-chosen proof command. The
packet should inspect existing sema name rendering and lookup helpers, add
structured key derivation helpers where stable AST/parser metadata already
exists, and keep existing string-keyed behavior intact.

## Watchouts

- Do not require HIR data-model cleanup in this plan.
- Do not remove `Node::name`, `TypeSpec::tag`, rendered diagnostics, mangled
  names, or link-name surfaces.
- Do not add testcase-shaped special cases or expectation downgrades.
- Keep structured helpers optional for AST nodes without stable text ids or
  qualified-name metadata.

## Proof

No code proof required for lifecycle-only activation. The first implementation
packet should at minimum run `cmake --build --preset default` plus the focused
frontend/parser/sema subset selected by the supervisor.

