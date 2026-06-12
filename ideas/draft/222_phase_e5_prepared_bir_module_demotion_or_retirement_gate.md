# 222 Phase E5 PreparedBirModule demotion or retirement gate

## Goal

Decide whether `PreparedBirModule` can be demoted from durable public prepared
artifact to private pass context, split into target-policy products, or finally
retired for selected surfaces.

This is the first phase that may rewrite or supersede draft 155, but only
after E0-E4 have produced enough evidence.

## Prerequisite

Do not open this draft until:

- E0 has closed with a field-by-field ownership map;
- accepted E1 semantic duplicate migrations have closed;
- accepted E2 public lookup/API contractions have closed;
- required E3 diagnostic/oracle replacements have closed;
- required E4 cross-target wrapper convergence work has closed;
- accepted baseline and string-authority state is non-regressive.

## Direction

E5 is still a gate before deletion. It must decide which outcome is safe:

- keep `PreparedBirModule` unchanged because blockers remain;
- demote selected fields to private pass context;
- split target-policy products out of the aggregate;
- rewrite draft 155 with precise deletion/demotion packets;
- open final implementation ideas for one field group at a time.

## In Scope

- `PreparedBirModule` construction and mutation sites.
- MIR/codegen, printer/debug, target-wrapper, oracle, and test consumers.
- Compatibility adapters needed during demotion.
- Final proof strategy for any deletion or demotion route.

## Out Of Scope

- Direct broad deletion of `PreparedBirModule`.
- Rename-only facade work.
- Combining retirement with unrelated backend feature work.
- Removing target policy, diagnostics, or fallback without proven replacements.

## Expected Output

The closure note must contain:

- whether draft 155 should be rewritten, opened, superseded, or kept blocked;
- the field groups ready for demotion or retirement;
- the field groups that must remain target/prepared policy or pass context;
- compatibility adapters required during transition;
- a final proof strategy including full-suite/baseline expectations;
- follow-up implementation ideas for each safe final packet.

## Reviewer Reject Signals

- Claiming whole-module retirement from selected route-reader evidence.
- Hiding the second IR behind a renamed wrapper.
- Losing prepared printer/dump visibility needed for regression diagnosis.
- Deleting public consumers before cross-target, diagnostic, fallback, and
  baseline proof is complete.
