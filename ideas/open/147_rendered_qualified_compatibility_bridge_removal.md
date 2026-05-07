# Rendered Qualified Compatibility Bridge Removal

Status: Open
Created: 2026-05-07

Parent Ideas:
- `ideas/closed/146_qualified_name_deferred_carrier_authority.md`

## Goal

Delete the remaining rendered qualified-name compatibility bridges after the
still-migrating qualified template and HIR paths consume structured carriers.

The target bridges are:

- `find_compatibility_key_from_rendered_qualified_spelling()`
- `intern_compatibility_key_from_rendered_qualified_spelling()`
- parser calls to `qualified_key_in_context()` that still accept one rendered
  `TextId` containing `::`
- shared rendered-spelling helpers such as `split_qualified_name_scope()` where
  they remain reachable from semantic lookup
- Sema/HIR fallback comments and routes that still describe no-structured-
  metadata compatibility bridges

## Why This Idea Exists

Idea 146 moved qualified-name authority to structured parser, Sema, and HIR
carriers and fenced the remaining rendered-spelling helpers as compatibility
boundaries. Step 8 also proved that removing the parser rendered-qualified
compatibility bridge still regresses qualified template/HIR compatibility
paths. That bridge deletion is narrower than the original authority migration
and needs a focused route that first migrates the remaining legacy carriers.

## In Scope

- Inventory the current compatibility bridge callers and classify which
  qualified template/HIR paths still lack complete structured metadata.
- Replace legacy rendered qualified `TextId` handoff with structured
  `QualifiedNameKey`, owner/name/domain metadata, or deferred carrier payloads.
- Remove parser compatibility key reconstruction once all semantic callers have
  structured metadata.
- Remove or restrict shared rendered-spelling helpers so semantic lookup cannot
  reach them.
- Add tests that fail if a qualified template or HIR path depends on splitting
  a rendered `A::B::C` spelling.

## Out Of Scope

- Reopening broad qualified-name authority migration already completed by
  idea 146.
- Changing C++ lookup rules beyond removing rendered-spelling compatibility
  bridges.
- Forcing all dependent lookup to finish before HIR.
- Rewriting diagnostics, dumps, display spelling, ABI names, or emitted symbol
  spelling unless they are incorrectly used as semantic lookup authority.
- Weakening tests or downgrading supported qualified template/HIR cases.

## Acceptance Criteria

- `find_compatibility_key_from_rendered_qualified_spelling()` and
  `intern_compatibility_key_from_rendered_qualified_spelling()` are deleted, or
  no semantic parser path can call them.
- Remaining qualified template and HIR paths carry structured metadata instead
  of one rendered `TextId` containing `::`.
- `split_qualified_name_scope()` is not reachable from semantic lookup
  authority; any surviving use is display-only and explicitly labeled as such.
- Focused tests cover the qualified template/HIR cases that previously
  regressed when the parser compatibility bridge was removed.
- Broader parser/Sema/HIR validation remains green.

## Reviewer Reject Signals

- A slice removes bridge names but keeps the same rendered `A::B::C` splitting
  behind a renamed helper.
- A route claims bridge removal while still accepting one `TextId` containing
  `::` as equivalent to a structured qualified-name carrier.
- Qualified template or HIR compatibility regressions are handled by weakening
  tests, marking cases unsupported, or rewriting expectations without replacing
  the carrier path.
- The implementation adds named-case shortcuts for one qualified template
  fixture instead of migrating the caller to structured metadata.
- Display or diagnostic spelling cleanup is claimed as semantic progress while
  the compatibility bridge remains reachable from lookup.
- Broad parser/Sema/HIR rewrites are mixed into the bridge-removal packet
  without a clear link to eliminating rendered-spelling semantic authority.
