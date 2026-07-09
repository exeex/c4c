# Prepared Incoming Stack Formal Authority

Status: Closed
Type: Implementation
Parent: `ideas/closed/644_rv64_object_route_stack_parameter_abi_residual.md`
Related:
- `ideas/closed/512_stack_passed_parameter_home_publication.md`
- `ideas/closed/644_rv64_object_route_stack_parameter_abi_residual.md`
- `review/reviewA.md`
Owning Layer: producer/prealloc publication of incoming stack-passed formal
authority
Queue Order: 52
Prerequisites: callee local stack-slot homes must remain distinct from caller
incoming stack ABI offsets.
Proof Surface: `src/20001017-1.c` now passes the allowlisted RV64 backend
object progress check using explicit incoming stack formal authority.

## Goal

Publish explicit prepared authority for stack-passed scalar formals that lets
RV64 consume the caller-stack incoming byte offset/address without deriving it
from formal order, ABI folklore, source syntax, local spill slots, or final
assembly.

## Why This Exists

Idea 644 proved the RV64 consumer route cannot safely recover stack-passed
formal incoming locations by walking `function.params`, applying ABI
size/alignment, and adding the callee frame size. The available prepared model
records `IncomingStackToHome` plus the callee local stack-slot home, but that
does not publish the caller-stack incoming offset/address as a first-class
authority. RV64 now fails closed on `src/20001017-1.c` until producer/prealloc
publishes that authority.

## In Scope

- Locate the producer/prealloc layer that owns stack-passed formal publication.
- Define an explicit prepared fact for each stack-passed scalar formal's
  incoming caller-stack byte offset/address.
- Keep that incoming authority separate from callee local spill-slot or home
  records.
- Teach prepared dumps or focused assertions to expose the new authority.
- Add focused negative coverage for missing, ambiguous, or local-home-only
  incoming stack formal authority.
- Once the authority exists, allow RV64 object-route consumers to use it
  without reintroducing offset reconstruction.

## Out Of Scope

- Reconstructing incoming offsets in RV64 from formal order, ABI size,
  alignment, source syntax, final assembly, or `stack_frame_bytes`.
- Treating `IncomingStackToHome` plus a local spill-slot home as sufficient
  caller-stack incoming authority.
- Generic call ABI rewrites, branch clobber-safety, terminator fragments,
  runtime policy, expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.
- Filename-specific handling for `src/20001017-1.c`.

## Acceptance Criteria

- Prepared producer/prealloc evidence names the incoming caller-stack
  offset/address for at least one stack-passed scalar formal, independently of
  the callee local home.
- Focused producer/prealloc tests prove complete explicit authority and reject
  missing, ambiguous, or local-home-only authority.
- RV64 consumer code consumes the explicit authority and does not derive
  incoming stack formal locations from formal sequence, ABI formulas, stack
  layout guesses, or final assembly.
- `src/20001017-1.c` advances past the missing explicit incoming stack formal
  authority diagnostic, or the route records a more precise producer/prealloc
  owner with current evidence.

## Completion Notes

Closed after the producer/prealloc model published explicit incoming stack
formal authority as `bir::CallArgAbiInfo::incoming_stack_offset_bytes` and
`PreparedFormalPublicationPlan::incoming_stack_offset_bytes`.

RV64 object consumers now load stack-passed scalar formals from that explicit
formal-publication fact. `PreparedValueHome::offset_bytes` remains the callee
local home/spill-slot offset and is used only for coherence or local
validation, not as incoming caller-stack authority.

Focused negative coverage rejects missing, local-home-only, and duplicate
branch-formal ambiguity cases. `review/reviewB.md` found no blocking drift or
testcase overfit; its ambiguity-coverage note was addressed before closure.

Close validation used the existing focused before/after logs:
`passed=8 failed=0 total=8` before and after. The proof includes the focused
producer/prealloc and RV64 object subsets plus the allowlisted
`src/20001017-1.c` RV64 backend object progress check.

## Reviewer Reject Signals

- Reject any RV64-side helper that computes incoming formal offsets by walking
  formals, applying ABI size/alignment, or adding `stack_frame_bytes`.
- Reject claiming progress by renaming `IncomingStackToHome` or local spill
  homes while still lacking a distinct caller-stack incoming offset/address
  fact.
- Reject literal expected load offsets such as `sp + 64` as positive proof
  unless the offset is traced to explicit prepared incoming authority.
- Reject named-case fixes for `src/20001017-1.c`, `bug`, `C`, or `fdC`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
