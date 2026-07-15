# LIR Native Vaarg Operand Carrier Foundation

Status: Open
Type: bounded frontend-LIR data-carrier prerequisite
Blocked Parent: `ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md`
Related Downstream Work: `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`, `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`

## Goal

Establish the smallest native, current-function typed operand/result carrier
contract that makes raw vaarg helper values structurally observable across the
AArch64 GP `gr_top` to `reg_addr`, AArch64 FP/alignment, and AMD64
register/stack chains.

## Why This Exists

783 completed its focused baseline and three-seam inventory, but cannot bind
native structural probes. `LirPhiOp::incoming` stores
`std::vector<std::pair<std::string, std::string>>`, while the named helper
GEP/load seam operands are constructed from `std::string` values. The probes
would therefore either recover identity from text or require changes that 783
explicitly excludes. This foundation isolates only the carrier question before
783 resumes its probe binding.

## In Scope

- Inventory the source-level operand/result carrier surfaces used by the three
  vaarg chains and test focused frontend-LIR structural-probe feasibility.
- Define and publish the smallest generic native, current-function operand or
  result carrier needed for the raw helper operands to survive from their
  authoritative producer to the immediate structural consumer in all three
  chains.
- Determine, with focused structural evidence, whether the value half of the
  existing vaarg PHI incoming representation must carry this foundation or
  whether helper operand publication alone reaches the required probe boundary.
- If the PHI incoming value carrier is necessary, limit it to transport of the
  already-published native current-function value authority; do not add PHI
  predecessor/edge semantics or verification.
- Add focused frontend-LIR structural coverage for the carrier contract and
  fail-closed missing, invalid, duplicate, or foreign authority where the
  selected carrier has an existing validation boundary.
- Publish a precise handoff that lets 783 resume Step 3 and add its three
  seam-specific structural probes.

## Out of Scope

- PHI predecessor/edge identity, PHI verification, or a claim of PHI
  completion; if the required PHI incoming value transport cannot remain a
  narrow data-carrier foundation, record it as an explicit successor.
- Raw-BIR/importer, backend, target lowering, MIR, emission, broad
  generic-expression redesign, textual recovery, and source/testcase-specific
  implementation unrelated to the generic carrier contract.
- 782 helper-field publication and 751's full PHI carrier/verifier work.

## Acceptance Criteria

- The three vaarg chains have a source-level carrier inventory and a focused
  frontend-LIR probe-feasibility result.
- One smallest native current-function operand/result carrier contract covers
  the raw helper values required at each chain's immediate consumer boundary,
  without names, labels, rendered text, instruction order, side tables, or
  result-name maps as authority.
- The outcome explicitly decides whether a narrow PHI incoming *value*
  transport is part of this foundation. If not supportable without PHI
  verification or predecessor/edge work, it names the exact separately scoped
  successor rather than absorbing that work here.
- Focused structural coverage proves the selected carrier contract and its
  fail-closed behavior; 783 may then resume exactly at Step 3.
- The accepted regression guard remains the matching baseline pair
  `test_before.log` and `test_after.log`; the focused command passes 1/1:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.

## Reviewer Reject Signals

- Reject generic migration, Raw-BIR/importer, backend, target lowering, MIR,
  emission, or broad expression redesign presented as carrier-foundation
  progress.
- Reject predecessor/edge identity, PHI verification, or full PHI completion
  being silently folded into a value-transport change.
- Reject text parsing, labels, printer output, instruction order, side tables,
  result-name maps, or testcase-shaped branches as native authority.
- Reject a GP-only, FP-only, or AMD64-only carrier claimed to unlock all three
  vaarg seams.
- Reject helper renames, expectation downgrades, weaker contracts, or retaining
  the string-only first-loss path behind a new abstraction name as capability
  progress.

## Resumption Record: generic call-result operand carrier blocker

Last accepted progress: Step 1, `Inventory carrier surfaces and prove
structural-probe feasibility`, accepted at commit `b96751b03`. Step 2, `Bind
the minimal generic carrier contract`, was interrupted. The active switch to
this 784 foundation is commit `969d81eaf`.

The Step 2 implementation attempt encountered the existing fail-closed GEP
native-index requirement. It was limited to unaccepted GP/FP subchain plumbing
and fully reverted when the AArch64 FP128 alignment route reached ptrmask:
`make_lir_call_op` retains its result as a string, erasing the native
`LirOperand`/`LirValueId` before `aligned_stack_ptr` can become the required
PHI input. Assigning authority at the PHI caller would fabricate it; changing
the generic call-result factory is outside this vaarg-only source scope.

Classification: `separate-blocker`. Active
`ideas/open/785_lir_call_result_operand_carrier_foundation.md` owns only the
minimal generic current-function call-result carrier for `make_lir_call_op` and
directly required support. It excludes generic expression redesign, PHI incoming
transport/verification, predecessor/edge identity, Raw-BIR/importer, backend,
target lowering, MIR, emission, and vaarg-specific helpers. If its direct
support type cannot carry `LirOperand` without unrelated generic call/argument
families, it must name that exact narrower successor.

Exact return point: after 785 accepts the direct generic call-result carrier,
reactivate 784 at Step 2 and retry its minimal generic carrier contract with
the call result available. Then preserve 784's distinct decision on whether
value-only PHI incoming transport is needed; do not treat 785 as PHI progress.
The remaining Step 2 action is otherwise unchanged.

Accepted proof after the reverted partial work:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1; full output
is `test_after.log`. No code or test change was accepted, and `test_before.log`
and `test_after.log` match.

## Resumption Record: direct call-result carrier accepted

785 is closed accepted. Commit `3b53451c0` converts only the AArch64 FP128
alignment ptrmask direct call at `vaarg.cpp:145` to `fresh_value(ctx)` plus the
existing `make_lir_call_op_with_return_type_ref`, retaining its ID through the
immediate typed GEP consumer. Its focused AArch64 long-double vaarg structural
probe proves the native call-result ID and pointer return type; the sibling HFA
ptrmask and unrelated generic call families remain untouched.

Accepted proof: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1. The
monotonic regression guard passed with before/after 1 passed, 0 failed, and no
new failures/timeouts. Preserve Step 1 accepted at `b96751b03`.

Exact return point: resume Step 2, `Bind the minimal generic carrier contract`,
with the FP128 ptrmask direct call-result carrier now available. Complete the
remaining three-chain carrier work and retain the separate decision on whether
value-only PHI incoming transport is required; no PHI verification or
predecessor/edge work is authorized by 785's closure.
