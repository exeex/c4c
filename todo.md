# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 6.2
Current Step Title: Establish complete publication semantic origin at the prepared owner

## Just Finished

- Step 6.2 separated both remaining owner-only consumer failures and received
  reviewer confirmation that the generic route remains aligned: AArch64 must
  consume only the attached `PreparedFunctionLookups` query, and publication
  semantic origin may be established independently of the narrower
  join-source-evidence applicability condition once complete prepared
  authority is proven.
- `backend_aarch64_current_block_join_routing` fails before query evaluation,
  but its first row is not a supported positive with a neutral attachment
  omission: `include_prepared_policy=false` leaves it without complete
  prepared routing authority while it retains historical Route-5-era positive
  expectations. Attaching an empty owner cannot authorize those bits.
- `backend_aarch64_instruction_dispatch` reaches the all-edge query for
  successor 2, routed value `511/2`. IncomingExpression has no applicable fact
  and returns `Mismatched`; its sole Source fact is also `Mismatched` because
  semantic origin is `Unknown`. The linked JoinTransfer has the matching edge
  destination but omits its authoritative `result`, so destination consistency
  correctly rejects it as incomplete. Completing
  `PreparedJoinTransfer.result` with the already-selected `%join.selected`
  destination is accepted as generic complete-edge fixture contract repair,
  not a weakened gate or testcase-shaped exception.

## Suggested Next

- Execute one narrow Step 6.2 fixture-and-proof packet:
  1. complete the immediate-select dispatch fixture's
     `PreparedJoinTransfer.result` with its existing selected/publication
     destination, while preserving the mismatched-destination fail-closed
     contract;
  2. keep the `include_prepared_policy=false` routing row explicitly
     policy-absent and negative/diagnostic at the routing-consumption boundary;
     do not preserve its positive routing expectations by attachment alone;
  3. add or run the focused Step 6.2 origin matrix covering immediate-backed
     publication, missing/incomplete and mismatched transfer results,
     unrelated operands, and multiple/parallel applicable facts before
     rerunning unchanged genuine policy-present supported rows and the backend
     subset.
- If the routing row is instead rebuilt as a supported positive, it must
  construct real complete prepared policy under the unchanged source intent;
  that is a policy-axis repair and must not be described as a semantics-neutral
  owner-attachment correction.

## Watchouts

- Do not reinterpret an absent lookup owner as attached authority. Do not
  weaken `prepared_join_transfer_destination_consistent`: the missing transfer
  result is a real incomplete-owner condition, not an owner-preparation defect.
- Reject the overfit route of attaching an empty owner to the
  `include_prepared_policy=false` row while retaining `{false, true, true}`:
  attachment proves lifetime/identity, not routing policy, and must not promote
  Route 5 diagnostics into authority.
- Preserve the owner-only AArch64 candidate unchanged and do not restore Route
  5 or target-local inference. Do not accept the publication-origin
  generalization until the focused matrix and unchanged genuine supported
  rows are green.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
- Build passed. The delegated 324-test backend subset passed 322 tests and
  failed `backend_aarch64_instruction_dispatch` and
  `backend_aarch64_current_block_join_routing`. The exact statuses above were
  reproduced with focused prepared-query instrumentation and then removed.
  Proof log: `test_after.log`.
