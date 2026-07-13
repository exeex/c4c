# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 12
Current Step Title: Converge cross-cutting observation and quarantine contracts

## Just Finished

- Plan Step 12 is complete. Diagnostics and rendering now borrow exact
  revision-bound public views, remain read-only and non-authoritative, and
  cannot feed text, cached output, audit facts, or observation errors back into
  compilation.
- The compatibility boundary is a closed, migration-only quarantine with an
  empty production manifest, exact field/writer/reader allowlists, same-key
  reads, monotonically decreasing checkpoints, and a zero-field/zero-reader
  deletion gate.
- The current `src/backend/legacy/` inventory, including `prealloc/`, has an
  explicit `Accepted`, `Reject`, or `Defer` disposition and exactly named new
  owner/boundary; duplicate routes and side-table mechanisms are rejected.
- The review template now walks the complete Raw -> Canonical -> preparation
  -> Pseudo/D4/D5 -> E1/E2/E3 -> E4/Allocated -> PreparedBir/MirReadyBirView
  boundary, including same-revision, failure-atomic, and anti-overfit checks.

## Suggested Next

- Execute Plan Step 13: perform the full cross-document architecture review,
  using the expanded review template and legacy ledger to check adjacency,
  ownership, revision keys, dispositions, and same-family proof across the
  subordinate contracts.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Step 13 should reject any observer, cache, quarantine adapter, legacy helper,
  or MIR route that can affect verifier outcome, product selection,
  publication, ordinary allocation, or capacity spill/reload.
- Preserve the distinction between distinct published Raw/Canonical/Pseudo
  revisions and the E4 rule that `AllocatedBir`, `PreparedBir`, and
  `MirReadyBirView` name one exact immutable allocated revision.
- `pipeline/README.md` required no adjacency correction in this packet; keep
  the root README as the sole normative total-order registry.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'mint.*(token|capability)|semantic authority|repair.*(revision|publication|allocation)|override.*verif|compatib.*decid|renderer.*decid' src/backend/bir/diagnostics/README.md src/backend/bir/compatibility/README.md src/backend/bir/LEGACY_COVERAGE.md src/backend/bir/REVIEW_TEMPLATE.md && rg -n 'read.only|non.authoritative|quarantin|revision|borrow|reject|owner|disposition|Raw|Canonical|Pseudo|Allocated|PreparedBir|MirReadyBirView' src/backend/bir/diagnostics/README.md src/backend/bir/compatibility/README.md src/backend/bir/LEGACY_COVERAGE.md src/backend/bir/REVIEW_TEMPLATE.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
