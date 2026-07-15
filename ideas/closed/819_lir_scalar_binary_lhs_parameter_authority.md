# LIR Scalar Binary-LHS Parameter Authority

Status: Closed — capability complete
Type: bounded LIR producer/schema/verifier prerequisite
Predecessor: `ideas/open/818_lir_next_body_parameter_authority_handoff.md`
Consumer after parent selection: `ideas/open/818_lir_next_body_parameter_authority_handoff.md`

## Goal

Publish one native, receiver-ready authority tuple for exactly a scalar
current-function parameter used as the `lhs` operand of `LirBinOp`. The tuple
must carry native value identity, parameter position, scalar type,
current-function owner, and ABI classification; it must not recover any fact
from presentation data.

## Why This Exists

818 Step 1 established that the nearest distinct scalar `LirBinOp` parameter
use currently reaches `preserve_exact_binary_operand` only as a raw display
operand. There is no native current-function parameter value/ABI definition
from which a receiver-ready row can be established. That producer authority is
a prerequisite outside 818's one-row selection/handoff scope.

## In Scope

- Trace only the scalar current-function parameter consumed as the `lhs` of a
  `LirBinOp`, and identify the native producer seam that can establish its
  value identity, position, scalar type, owner, and ABI classification.
- Publish that single tuple through the minimum LIR schema/producer path, with
  explicit operand-role association and no alternate operand or parameter form.
- Verify the tuple transactionally and add focused same-feature producer
  coverage for the selected positive form and malformed authority.
- Hand the exact tuple, permitted ABI class, reject boundary, and focused proof
  back to 818. Do not receive it into Raw-BIR in this idea.

## Out Of Scope

- Raw-BIR destinations, importer/dispatcher/receiver tests, or 734 changes;
- `LirBinOp.rhs`, another binary operator/form, API-wide parameter publication,
  ABI-wide conversion, presentation recovery, or a multi-form sweep;
- `param_slots`, rendered parameter names/signatures, `LirOperand::raw`,
  diagnostics, or testcase identity as authority; and
- closed 795 GEP-index, closed 817 direct-pointer GEP-base, byval-memcpy, or
  any previously accepted direct-pointer/index form.

## Acceptance Criteria

- Exactly the selected scalar `LirBinOp.lhs` current-function parameter form
  has a checked native value/position/type/owner/ABI contract.
- The verifier rejects missing, invalid, duplicate, foreign-owner,
  wrong-position, non-scalar/type-incoherent, ABI-incoherent, role-mismatched,
  and display-derived forms before any downstream use.
- Focused positive and malformed-authority producer proof is accepted, and the
  durable handoff tells 818 only what it may use to re-evaluate a
  receiver-ready row.

## Reviewer Reject Signals

- Reject any authority reconstructed from `param_slots`, rendered `%p.<name>`,
  signatures, diagnostics, `LirOperand::raw`, or `preserve_exact_binary_operand`
  string matching.
- Reject accepting `rhs`, a non-scalar parameter, a foreign/current-owner
  mismatch, a wrong type or position, or a nonselected ABI class under a
  renamed/generalized helper.
- Reject reuse of closed 795 GEP-index or 817 direct-pointer contracts, or a
  byval-memcpy fact, as proof for this `LirBinOp.lhs` form.
- Reject Raw-BIR/importer/dispatcher/receiver edits, API-wide parameter work,
  expectation downgrades, testcase-shaped shortcuts, or broad rewrites claimed
  as this one producer capability.

## Parent Return Point

After this idea has accepted its one-form producer proof and exact handoff,
reactivate 818 at **Step 1 - Trace and select one body-parameter use authority
row**. Re-evaluate only whether this published scalar `LirBinOp.lhs` tuple is
receiver-ready, then select/trace that new row before continuing 818 Steps 2–3
as applicable. Do not receive Raw-BIR inside this idea.

## Completion Record

- Disposition: capability complete. The bounded producer prerequisite is
  accepted; this idea does not claim a Raw-BIR receipt or any receiver work.
- Accepted producer tuple: `LirNativeBodyParameterAbi::DirectScalar` exists
  alongside `DirectPointer`. `init_fn_ctx` publishes exactly one
  `LirCurrentFunctionBodyParameterDefinition` for a plain fixed scalar
  current-function parameter (integer/floating base, `ptr_level == 0`, with no
  array, vector, reference, or function-pointer form). It carries the native
  `LirValueId`, parameter index, LLVM type, current-function owner, and
  `DirectScalar` ABI. `emit_binary_rval_operand` binds that matching native
  definition only to `LirBinOp.lhs` through
  `LirScalarBinaryLhsParameterAuthority` with role `Lhs`.
- Reject boundary: reject absent, invalid, duplicate, foreign-owner,
  wrong-role, out-of-range, type/ABI-incoherent, non-scalar, and lhs
  value/type-mismatch forms. Verification requires a unique native
  direct-scalar owner; exact current-function definition, parameter
  index/type/owner/ABI agreement; matching lhs SSA/value/type; and a plain
  fixed scalar form. Presentation data, `param_slots`, rendered text, and
  `LirOperand::raw` are not authority.
- Accepted implementation and proof: `b16935c69` (`lir: publish scalar binary
  lhs parameter authority`); `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee
  test_after.log` passed 6/6. Matching `^backend_` `test_before.log` and
  `test_after.log` regression guard passed with no new failures; the
  allow-non-decreasing policy applies because this target exercises internal
  cases.
- Parent return: resume 818 at Step 1 — `Trace and select one body-parameter
  use authority row` — and re-evaluate only whether this exact scalar
  `LirBinOp.lhs` tuple creates a receiver-ready row. 818 Steps 2–3 remain
  untouched. No Raw-BIR receipt occurred here.
