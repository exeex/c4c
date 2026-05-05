# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Post-119a6ef Consumer Repairs

## Just Finished

Lifecycle switch created the TypeSpec identity normalization route and parked
the active idea 141 field-removal runbook.

Initial Step 1 inventory from commits after
`119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8` found the repeated pattern:
consteval, codegen aggregate lookup, and owner-key helpers each had to recover
from raw `TypeSpec::tag_text_id` being compared against a `TextTable` the
producer did not own. The same route also appears at parser typedef resolution,
where a stored stale TypeSpec can overwrite the source typedef TextId unless
the producer boundary preserves it.

## Suggested Next

Continue Step 1 by classifying direct `tag_text_id` lookups in
`src/frontend/hir`, `src/frontend/sema/consteval.*`, and
`src/codegen/shared/llvm_helpers.hpp` as one of: already normalized domain key,
explicit no-metadata compatibility, or residual route drift. The first likely
implementation target is Step 2 parser TypeSpec producer identity, because the
current failing guard is narrow and proves the normalize-at-production rule.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- Keep idea 141 parked until normalized identity boundaries are stable.

## Proof

Lifecycle-only switch; no build proof required for the planning files.
