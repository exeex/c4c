Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Survey Current Header And Include Boundaries

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/bir-agent-index-header-hierarchy.md`.

## Suggested Next

Execute `plan.md` Step 1 by surveying `src/backend/bir/lir_to_bir.hpp`,
LIR-to-BIR implementation includes, and external callers. Record the exported
API declarations that must stay public and the private lowering declarations
that should move to `src/backend/bir/lir_to_bir/lowering.hpp`.

## Watchouts

- This initiative is structural; do not change lowering semantics or testcase
  expectations.
- Do not edit `ideas/open/bir-agent-index-header-hierarchy.md` for routine
  execution notes.
- Do not create one header per implementation file.

## Proof

Activation-only lifecycle change. No build or test proof required yet.
