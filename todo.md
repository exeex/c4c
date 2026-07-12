# Current Packet

Status: Active
Source Idea Path: ideas/open/723_pre_regalloc_value_constraint_carrier_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide semantic owner and schema

## Just Finished

- Plan Step 2 compared BIR, prepared semantic, liveness, and regalloc ownership in `docs/pre_regalloc_value_constraints/02_semantic_owner_and_schema.md` and selected a bounded split: authenticated ingress and target validation in prepared semantic state, normalization in `PreparedAllocationConstraint`, and enforcement in common regalloc.
- The decision rejects unrestricted BIR/value-name register controls. It specifies stable identity, typed provenance, structured target identity, class/width legality, conflict handling, failure behavior, and constraint-aware candidate filtering without encoding the joined-branch fixture.

## Suggested Next

- Execute Plan Step 3 by defining a route-independent positive semantic program, adjacent negative matrix, and deterministic follow-up boundary.

## Watchouts

- A later implementation must constrain every normal and eviction candidate-pool pass from the same normalized row. Publishing requests without enforcement, or exposing arbitrary value-name-to-register preparation options, would preserve the original failure behind a new carrier.

## Proof

- No build was required for this documentation-only packet.
- Ran repository `rg` checks for every cited owner, type, phase, target-identity helper, candidate-selection symbol, and constraint field; all cited paths and symbols resolved. `git diff --check` passed.
