Status: Active
Source Idea Path: ideas/open/702_residual_stack_authority_revisit_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind Follow-Up Routing

# Current Packet

## Just Finished

Step 2 defined the prepared-owned positive producer threshold required before
any future reactivation of ideas 647 or 655.

A future residual stack-destination fan-in implementation may start only after
a named prepared/prealloc producer family proves all participating facts at the
consumer program point:

- destination value identity: the exact destination value id/name targeted by
  the fan-in;
- destination home: the stack slot or destination home authorized for that
  value;
- destination storage kind: explicit stack-destination storage, not inferred
  from route success or final assembly;
- source value/home: every participating source value and its selected home at
  the fan-in point;
- selected move evidence: either a selected move bundle or selected move
  resolution that ties the participating sources to the destination;
- selected freshness: freshness for the selected destination-authority family,
  not only `BranchStackLoadSource`;
- stack object or aggregate source authority: stack object identity or
  aggregate stack-source authority when the selected family depends on an
  aggregate or stack-object source;
- producer fact: one explicit non-637 family such as ordered final-state,
  mutual-exclusion, explicit merge, aggregate stack-source, or another named
  prepared producer family;
- MIR fail-closed statuses: missing, unsupported, ambiguous, stale, and
  bundle-versus-move mismatched authority must remain unavailable before any
  RV64/materialized stack path.

Eligible producer families for a future reactivation are limited to prepared
producer seams that can publish the facts above, for example ordered
final-state authority, mutual-exclusion authority, explicit merge authority,
aggregate stack-source authority, or another named prepared-owned destination
authority family. `PreparedBranchStackLoadAuthority` remains valid evidence for
branch stack-load source freshness only; it is not a stack-destination fan-in
producer threshold by itself.

Insufficient route compatibility evidence remains rejected as direct stack
authority: Route 4 publication rows, Route 5 status or agreement rows, Route 7
comparison validation rows, `RouteIndexReferenceFacade`, route dumps, dump
labels, expected output, expectations, allowlists, unsupported-marker policy,
timeout/pass-fail accounting, diagnostic wording, final assembly, ABI or
runtime behavior, testcase identity, source order, value ids, block labels,
move-vector order, frame-slot existence, source freshness alone, and
string-label pointer authority.

Required positive proof surface before any future 647/655 implementation idea
can start: one focused prepared/prealloc contract or backend probe for the
selected family that publishes the named producer fact with destination value,
destination home/storage kind, participating source value/homes, selected move
bundle or move resolution, selected freshness, and stack object or aggregate
source authority where applicable.

Required negative fail-closed proof surface before any future 647/655
implementation idea can start: one focused prepared/prealloc contract or
backend probe proving missing, unsupported, ambiguous, stale, or
bundle-versus-move mismatched destination authority remains unavailable and
does not permit RV64 target materialization.

## Suggested Next

Start Step 3 by binding follow-up routing: map any eligible positive seam to
one named future producer family, keep unrelated or still-negative seams
rejected, and leave ideas 647 and 655 parked unless this threshold is met.

## Watchouts

- Keep ideas 647 and 655 parked unless positive prepared producer evidence
  above route dumps is identified.
- Do not treat Route 4, Route 5, Route 7, facade status, dump labels,
  expectations, allowlists, or diagnostic wording as stack authority.
- Do not start RV64 materialization or residual stack authority implementation
  from this prerequisite runbook.
- Do not let the branch stack-load authority slice stand in for
  stack-destination fan-in prerequisites; it proves only its own family.
- A future Step 3 routing note should not invite implementation from route
  compatibility evidence; it should name the producer family and proof surfaces
  that would unblock a later lifecycle packet.

## Proof

No build/test proof required for this todo-only Step 2 threshold definition.
