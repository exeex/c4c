# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Reconcile legacy and core dispositions

## Just Finished

- Plan Step 11 reconciled the retained legacy ledger so every accepted
  capability names one present architectural producer/owner and later stages
  are identified only as consumers or boundaries. The former generic view
  family is split across concrete core, comparison, memory-effects,
  provenance, and publication owners; the combined control-flow view is
  rejected because no single producer owns all of its mixed facts.
- The nonexistent `analysis/alias` owner was removed. Conservative origin/path
  queries belong to indexed `analysis/provenance`; stored freshness and a
  standalone alias authority are explicitly rejected.
- The core legacy map now keeps out-of-SSA/copy resolution at D5, intervals and
  interference at E1, abstract homes at E2, abstract spill objects and explicit
  `Spill`/`Reload` at E3, publication at E4, and only concrete one-to-one
  mapping at F1 MIR.

## Suggested Next

- Execute Plan Step 12, "Reconcile the root and run the complete documentation
  proof."

## Watchouts

- Step 12 should confirm the complete root index and inventory after this
  ledger reconciliation; the ledger is an architecture disposition record,
  not an implementation-completion claim.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'analysis/alias|later-stage MIR data|MIR.*(owns|performs).*(liveness|allocation|spill/reload|out-of-SSA|copy resolution|copy scheduling)|homes.*BIR/MIR' src/backend/bir/LEGACY_COVERAGE.md src/backend/bir/core/README.md src/backend/bir/README.md src/backend/mir/README.md && rg -n 'Accepted|Reject|Defer|analysis/memory|D5|E1|E2|E3|E4|interval|interference|abstract home|Spill|Reload|MIR|one-to-one|owner' src/backend/bir/LEGACY_COVERAGE.md src/backend/bir/core/README.md src/backend/bir/README.md src/backend/mir/README.md`.
- Accepted-row owner audit passed: every ledger row whose disposition contains
  `Accept` has literal `Producer/owner:`, and the concrete owner documents for
  the split view families and all other retained capabilities exist.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
