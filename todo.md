Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Canonical String Authority

# Current Packet

## Just Finished

Activation created `plan.md` and this executor-compatible `todo.md` skeleton
for Step 1.

## Suggested Next

Start Step 1 by inventorying string fields, string-keyed lookups, equality
comparisons, and hash inputs in `src/frontend/sema/canonical_symbol.hpp` and
`src/frontend/sema/canonical_symbol.cpp`.

## Watchouts

- Do not treat rendered spelling, ABI mangling text, or debug formatting as
  semantic authority for covered metadata paths.
- Do not replace a string key with a raw `TextId` when owner, scope, template,
  or nominal-domain context is required.
- Document any retained rendered-string fallback with owner, limitation, and
  removal condition.

## Proof

Lifecycle-only activation. No build or test proof was run.
