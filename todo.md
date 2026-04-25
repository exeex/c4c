Status: Active
Source Idea Path: ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Legacy Type Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for the active report-only audit. The review
artifact now records the plan's primary LIR/backend legacy type surfaces with
code location, reader/writer ownership, consumer role, and candidate blocker
class.

## Suggested Next

Delegate Step 2 to classify blocker ownership from the Step 1 inventory,
separating BIR/backend, HIR-to-LIR layout, raw type-ref, bridge-required,
proof-only, printer-only, and planned-rebuild surfaces.

## Watchouts

- This is a report-only audit; do not change implementation files.
- Classify MIR/aarch64 legacy consumers as `planned-rebuild` only.
- Do not treat MIR migration as required cleanup or as a blocker for BIR/LIR
  follow-up work.
- `module.type_decls` is still active BIR aggregate-layout input, while
  `struct_decls` is printer authority plus proof shadow.
- `StructuredLayoutLookup::legacy_decl` remains HIR-to-LIR layout authority for
  selected size/align and payload decisions.

## Proof

Report-only packet; no build required. Sanity proof command:
`git diff --name-only`, expected to show only `todo.md` and
`review/112_lir_backend_legacy_type_surface_readiness_audit.md`.
