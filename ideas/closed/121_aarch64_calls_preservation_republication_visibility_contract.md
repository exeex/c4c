# 121 AArch64 Calls Preservation Republication Visibility Contract

## Goal

Add focused route visibility for calls preservation and republication lowering
before any owner extraction or fact movement is attempted.

## Why This Exists

Idea 118 classified preservation and republication lowering as
`contract-needed`. The cluster has prepared inputs, but it crosses before-call
prior-preservation moves, call preserve-effect publication, and
stack-to-register republication records. Without focused dumps or tests, a
movement route could hide side-effect changes or reopen idea 93
stack-preserved source authority.

## In Scope

- Route-visible coverage for `PreparedCallPreservedValue`, prepared
  preservation routes, endpoint stack/register data, indexed first
  stack-preserved values, current prepared value homes, and emitted-register
  state used by calls lowering.
- Tests or dumps that cover preserve-effect publication both enabled and
  disabled.
- Coverage for prior stack-preserved argument consumption before a call.
- Coverage for stack-to-register republication through the same route.
- Documentation in test names, dumps, or comments that distinguishes prepared
  preservation authority from AArch64-local emission.

## Out Of Scope

- Extracting a preservation owner before the visibility contract exists.
- Reopening stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Moving AArch64 callee-saved register spelling, frame-slot store line
  spelling, preservation-home publication records, or ABI/local effect
  resources into shared code.
- Reworking call preserve-effect semantics beyond what is needed to expose the
  existing route.
- Broad calls lowering cleanup or line-count contraction.

## Likely Files

- Existing backend prepared-printer or route-dump tests for call preservation,
  if present
- Existing backend/AArch64 instruction dispatch tests for call preservation and
  republication paths
- `src/backend/mir/aarch64/codegen/calls.cpp` only as a consumer/proof target
  if narrow instrumentation or expected output requires it
- Shared prepared call-preservation printer or lookup files only if visibility
  currently lacks a source

## Owner Boundary

This idea does not move ownership. It proves the boundary. Shared/prepared code
owns preservation route facts and endpoint data. AArch64 calls lowering owns
callee-saved register spelling, frame-slot store lines, and call-boundary
machine records once those facts are known.

## Proof Route

- Identify the narrowest existing tests or dumps that can show preservation
  publication state.
- Add focused visibility for preserve-effect publication enabled and disabled.
- Add or update focused AArch64 route coverage for prior stack-preserved
  argument consumption and stack-to-register republication.
- Run the supervisor-selected backend/AArch64 subset covering call
  preservation, prepared printer or dumps, and call-boundary movement.

## Acceptance And Completion Criteria

- Reviewers can see whether preservation publication is enabled or disabled
  for the tested route.
- Reviewers can see prior stack-preserved argument consumption and
  stack-to-register republication using prepared preservation facts.
- The route records enough evidence to decide whether a later preservation
  owner extraction is safe.
- No implementation route claims owner movement or shared fact migration as
  progress before the visibility contract is proven.

## Reviewer Reject Signals

- The route extracts or rewrites preservation lowering before proving route
  visibility.
- The route reselects stack-preserved sources, endpoint stack storage, or
  frame-slot homes instead of consuming idea 93 ownership.
- The proof covers only preserve-effect publication enabled or only disabled,
  but not both when both modes remain supported.
- The proof covers only one prior-preserved argument shape and ignores
  stack-to-register republication.
- Tests are weakened, supported paths are marked unsupported, or expectation
  rewrites hide preservation side-effect changes.
- The diff claims capability progress through helper renames or line-count
  reduction with no new route-visible preservation evidence.

## Closure Note

Closed after Step 5 acceptance review. The final route proved the visibility
contract without owner extraction or authority movement: preserve-effect
publication is visible in both enabled and disabled modes, prior
stack-preserved argument consumption uses prepared preservation facts instead
of local stack-source reselection, and stack-to-register republication is
visible through prepared endpoint data. Reviewer report
`review/idea121_preservation_visibility_review.md` found no drift or overfit;
its missing-`test_after.log` blocker was resolved by regenerating the focused
four-test proof and passing the matching regression guard with 4/4 before and
4/4 after, no new failures. Follow-up ideas 122 and 123 remain open as
separate shared prepared-query initiatives.
