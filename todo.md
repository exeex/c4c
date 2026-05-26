Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validation and Closure Notes

# Current Packet

## Just Finished

Step 5: Validation and Closure Notes completed the closure-note packet by
recording the final retired-path mapping for the active fallback-retirement
route:

- Retired local aggregate address publication fallback:
  `calls_dispatch_bridge` no longer rederives an address from legacy local
  stack/value-source state when the call argument source selection is absent.
  Replacement authority is
  `PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`,
  populated before AArch64 lowering with the selected stack offset and pointer
  delta. AArch64 consumes that prepared materialization to publish the local
  aggregate object address.
- Retired direct/zero-offset local frame address reconstruction:
  direct `FrameSlotAddress` and derived
  `LocalFrameAddressMaterialization` facts are now the only supported prepared
  authorities for the owned local-address path. Missing, absent, or internally
  incomplete selected-source records fail closed with `MissingValueAuthority`
  or the prepared local-frame-address diagnostic; they do not fall back to
  source rederivation.
- Unreachable non-local local-address fallback entries:
  byval aggregate arguments and `va_start` intrinsic arguments are intentionally
  outside the local aggregate address publication route. Their guard coverage
  proves they do not enter the retired local-address fallback rather than
  mapping to a replacement local-address authority.
- Retired indirect byval register-lane payload-store scan:
  `calls_byval_aggregates` no longer reconstructs register-lane payload bytes
  from target-local BIR store scans, register homes, or frame-slot homes.
  Replacement authority is a complete
  `PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane` record consumed
  through the prepared-source adapter.
- Retired indirect byval register payload publication fallback:
  register-home lanes now publish payload from the prepared
  `ByvalRegisterLane` source fields: payload slot, payload offset, payload
  size, alignment, lane extent, and source instruction facts. Missing or wrong
  selected-source records fail closed with `MissingValueAuthority`.
- Retired indirect byval stack lane payload publication fallback:
  stack-home and overflow lanes use the same prepared `ByvalRegisterLane`
  payload authority, with prealloc filling the selected lane payload
  slot/offset/size/alignment from prepared addressing facts before lowering.
  Missing, incomplete, or mixed stack/register lane source records fail closed
  instead of deriving bytes from old frame-slot homes or payload-store
  reconstruction.
- Remaining intended indirect byval address publication:
  large indirect byval aggregate address materialization remains a prepared
  frame-address path, not the retired lane-payload fallback. Its absent or
  incomplete prepared facts diagnose rather than recovering payload bytes from
  target-local state.

No owned route remains broad or target-local for the retired local aggregate
address publication path or the retired indirect byval register/stack lane
payload publication path.

## Suggested Next

Hand this Step 5 closure state to the plan owner for lifecycle closure decision
for `ideas/open/17_aarch64_absent_selection_fallback_retirement.md`.

## Watchouts

- This packet is lifecycle/audit note work only; no implementation, test,
  `plan.md`, source idea, review artifact, or proof-log edit was made.
- `review/absent_selection_fallback_step5_review.md` found no testcase-overfit
  or expectation weakening, but required this final mapping note before
  closure.
- Broad validation is acceptable as matched-regression evidence only with the
  known `c_testsuite_aarch64_backend_src_00181_c` failure called out; do not
  recast that known failure as new fallback-retirement work.

## Proof

No new build/test proof was delegated for this closure-note packet.

Supervisor Step 5 validation already captured: broader
`backend_|c_testsuite_aarch64_backend_` before/after logs both pass 381/382 and
fail only the known `c_testsuite_aarch64_backend_src_00181_c`; focused backend
subsets for the fallback-retirement route were green. Canonical proof logs:
`test_before.log` and `test_after.log`.
