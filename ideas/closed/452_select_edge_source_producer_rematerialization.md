# Select-Edge Source-Producer Rematerialization

Status: Closed
Type: Prepared move-bundle materialization authority idea
Parent: `ideas/closed/450_select_result_branch_publication.md`
Source Evidence: `build/agent_state/450_step4_residual_disposition/`
Close Evidence: `build/agent_state/452_step4_residual_disposition/disposition.md`
Owning Layer: Prepared source-producer facts for predecessor-edge
out-of-SSA move bundles
Closed Disposition: Complete for register/immediate select-edge
source-producer rematerialization; stack-slot pointer dependency routed to
`ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md`

## Goal

Define and consume explicit prepared authority for rematerializing
source-producer dependency chains at select-edge out-of-SSA move sites, so a
predecessor edge can materialize an incoming select value without copying a
successor/join-block temporary that is not available on that edge.

## Why This Exists

Idea 450 reached a precise blocker in `20010329-1`. The selected result `%t22`
feeds a valid branch condition, but one incoming value is `%t18`, a compare
defined in the successor/join block over `%t15` and `%t17`. `%t17` is an
`inttoptr` result with a stack-slot pointer home. The predecessor-edge move
site cannot safely copy `%t18`; it must either rematerialize the compare/cast
dependency chain from explicit prepared source-producer facts or fail closed.

## In Scope

- Audit the `20010329-1` select-edge move-bundle failure and nearby prepared
  source-producer records for `%t22`, `%t18`, `%t17`, and their operands.
- Define the prepared facts required to authorize edge rematerialization:
  source-producer identity, dependency operands, edge availability, dominance
  or placement proof, value homes, cast/compare operation identity, and
  target register/storage constraints.
- Classify admissible rematerialization shapes, including whether pointer
  compare dependencies with `inttoptr` operands and stack-slot pointer homes
  can be materialized by a bounded producer/consumer contract.
- Add focused coverage for accepted rematerialization facts and fail-closed
  missing, incoherent, unavailable, non-dominating, or unsupported dependency
  chains.
- Select one narrow semantic implementation packet only after the authority
  contract is explicit.

## Out Of Scope

- Direct select-result branch publication without edge rematerialization
  authority, closed by idea 450.
- Stack-home branch operand and condition materialization not required by the
  selected edge rematerialization contract, tracked separately by
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Pointer-value memory provenance publication, including external-linkage
  formal provenance blocked by the closed no-external-caller source branch.
- Generic instruction-side lowering, including current `20000622-1`.
- Raw-BIR reconstruction, testcase-shaped matching, filename/function/block
  matching, expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- `20010329-1` select-edge source-producer dependencies are classified by
  accepted rematerialization authority versus exact missing producer,
  placement, home, or target constraints.
- Accepted rematerialization, if any, consumes explicit prepared facts and is
  covered by focused pass and fail-closed tests.
- A successor/join-block temporary is never copied on a predecessor edge unless
  the source value is proven available there; otherwise the dependency chain is
  rematerialized from authority or the route fails closed.
- Remaining stack-home, pointer-provenance, instruction-side, or storage
  residuals are routed to their own source ideas instead of being folded into
  this one.

## Completion Notes

Steps 1 and 2 classified the select-edge source-producer shape and defined the
first bounded contract. Step 3 found no implementation change was needed for
the selected register/immediate packet: existing object emission already
rematerializes prepared select-edge compare sources whose dependencies are
register or immediate compatible, and focused object tests cover accepted and
fail-closed cases. Step 4 re-probed `20010329-1` and confirmed that the
remaining `%t18 -> %t22` failure is not more register/immediate
rematerialization.

The unresolved representative row depends on `%t17 = inttoptr i32 %t16 to ptr`
with `%t17` stored in stack slot `slot_id=2 offset=16`. That stack-slot pointer
dependency was explicitly rejected from idea 452's selected packet. Copying
`%t18` remains unsound because `%t18` is defined in the successor/join block
after the predecessor-edge move site.

Idea 452 is closed as complete for register/immediate select-edge
source-producer rematerialization. The remaining stack-slot pointer dependency
inside select-edge materialization is routed to
`ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md`.
General stack-home branch operand/condition work remains separately tracked by
`ideas/open/451_stack_home_branch_operand_materialization.md`.

## Validation

- Step 4 backend proof passed before log roll-forward:
  `cmake --build build -j2` plus
  `ctest --test-dir build -j2 --output-on-failure -R '^backend_'`.
- Step 4 `git diff --check` passed.

## Reviewer Reject Signals

- Reject copying `%t18` or any successor/join-block compare result on a
  predecessor edge without edge-availability proof.
- Reject reconstructing source-producer chains from raw BIR shape, block
  order, filenames, function names, or one prepared dump layout.
- Reject testcase-shaped handling keyed to `20010329-1`, `%t22`, `%t18`,
  `%t17`, one stack slot, or one block name.
- Reject broad stack-home, pointer-provenance, or generic instruction lowering
  work presented as select-edge rematerialization progress.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
