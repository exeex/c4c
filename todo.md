# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory constructor uses and choose warning proof

## Just Finished

- Activation complete: 760 is the sole active route. 759 is closed, and no
  migration implementation has been performed in this packet.

## Suggested Next

- Step 1 audit: inventory relevant LIR ref string-constructor uses, classify
  closed-set versus dynamic runtime-text boundaries, and choose a narrow
  warning/compile proof strategy without migrating callers.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- Keep dynamic aggregate, vector, struct, and function text paths supported.
- Do not weaken verifier behavior, tests, or expected output.

## Proof

- Lifecycle activation only; no code or test artifacts were changed.
