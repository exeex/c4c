Status: Active
Source Idea Path: ideas/open/702_residual_stack_authority_revisit_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Positive Producer Threshold

# Current Packet

## Just Finished

Step 1 established the post-700 residual stack authority revisit baseline.

The completed idea 700 evidence is positive prepared producer evidence only for
`PreparedBranchStackLoadAuthority`: branch stack-load source freshness at a
branch terminator, consumed through the prepared MIR query/view
`query_prepared_mir_branch_stack_load_authority()` and
`PreparedMirFunctionView::branch_stack_load_authority()`, with fail-closed
behavior for missing or unavailable authority.

That evidence is not enough to resume ideas 647 or 655. Those parked ideas are
about residual non-637 stack-destination fan-in, not branch stack-load source
freshness. They still need positive prepared/prealloc producer evidence for a
stack-destination authority family at the failing consumer program point.

Missing residual stack-destination fan-in prerequisites:

- destination value identity;
- destination home;
- destination storage kind;
- source value/home for participating fan-in sources;
- selected move bundle or move resolution;
- selected freshness applicable to the destination fan-in family, not only
  `BranchStackLoadSource`;
- stack object or aggregate stack-source authority where applicable;
- ordered final-state, mutual-exclusion, explicit merge, or another named
  non-637 destination-authority producer fact;
- MIR fail-closed statuses for missing, unsupported, ambiguous, stale, and
  bundle-versus-move mismatched destination authority before target
  materialization.

Rejected as direct stack authority: Route 4 publication rows, Route 5 status
or agreement rows, Route 7 comparison validation rows,
`RouteIndexReferenceFacade`, route dumps, dump labels, expected output,
expectations, allowlists, unsupported-marker policy, timeout/pass-fail
accounting, diagnostic wording, final assembly, ABI or runtime behavior,
testcase identity, source order, value ids, block labels, move-vector order,
frame-slot existence, source freshness alone, and string-label pointer
authority.

## Suggested Next

Start Step 2 by converting this baseline into an explicit prepared-owned
positive producer threshold for any future 647 or 655 reactivation.

## Watchouts

- Keep ideas 647 and 655 parked unless positive prepared producer evidence
  above route dumps is identified.
- Do not treat Route 4, Route 5, Route 7, facade status, dump labels,
  expectations, allowlists, or diagnostic wording as stack authority.
- Do not start RV64 materialization or residual stack authority implementation
  from this prerequisite runbook.
- Do not let the branch stack-load authority slice stand in for
  stack-destination fan-in prerequisites; it proves only its own family.

## Proof

No build/test proof required for this todo-only Step 1 baseline packet.
