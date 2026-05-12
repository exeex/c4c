Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Sema Compatibility Routes

# Current Packet

## Just Finished

Activated `ideas/open/199_sema_legacy_compatibility_retirement.md` as the
active lifecycle plan. `plan.md` now contains the Sema compatibility retirement
runbook, and this `todo.md` points at Step 1.

## Suggested Next

Begin Step 1 by inventorying Sema-owned rendered compatibility routes and
recording the first narrow conversion or fencing target here.

## Watchouts

- Keep the work Sema-owned; do not expand into parser, broad HIR lowerer, BIR,
  LIR, backend, or full consteval-engine rewrites.
- Do not claim progress through helper renames, diagnostics-only changes, or
  expectation rewrites.
- Covered metadata-rich Sema misses must not recover through rendered enum,
  consteval function, NTTP, type-binding, local-const, or struct-def maps.
- Retained Sema bridges need `legacy` or `deprecated` comments with owner,
  limitation, and removal condition.

## Proof

Lifecycle activation only; no code validation required.
