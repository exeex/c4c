Status: Active
Source Idea Path: ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Build The Retirement-Readiness Checklist

# Current Packet

## Just Finished

Completed Step 4 by creating
`docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md` as
the durable retirement-readiness checklist artifact for prepared diagnostic and
oracle surfaces.

The checklist summarizes contraction blockers by surface, route family, and
consumer group; names compatibility adapter needs and retained-oracle
rationales; splits future implementation candidates by diagnostic/oracle
surface; and records reject signals for expectation weakening, output-only
relabeling, testcase-shaped diagnostic shortcuts, production-success claims,
and unlabeled prepared-as-route authority.

The artifact keeps prepared contraction prerequisites explicit without claiming
contraction readiness. Each future implementation candidate is scoped to one
surface, route family, or consumer group and must preserve existing prepared
oracle strength until equivalent route-native positive, negative, ambiguous,
mismatch, and fallback coverage exists.

## Suggested Next

Execute Step 5 validation consistency: re-read source idea 194 acceptance
criteria and reviewer reject signals, confirm the checklist satisfies the
planning scope, verify only docs/lifecycle files changed, and ensure the
artifact does not claim prepared API contraction or route capability progress.

## Watchouts

The checklist intentionally retains all prepared surfaces as blockers/oracles.
Do not use it as approval to delete, privatize, relabel, or weaken prepared
printer, dump, route-debug, wrapper, oracle, c_testsuite, or baseline
expectations. Step 5 should check wording for accidental contraction-readiness
claims before lifecycle close or follow-up idea generation.

## Proof

Docs/lifecycle-only packet; no build required and no `test_after.log` produced.
Verification was creation of the durable checklist artifact from the Step 1-3
notes in `todo.md` and source idea 194, plus this canonical `todo.md` Step 4
handoff update.
