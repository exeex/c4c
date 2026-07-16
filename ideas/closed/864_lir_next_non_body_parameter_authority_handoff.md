# LIR Next Non-Body-Parameter Authority Handoff

Status: Closed
Type: producer/schema/verifier handoff for one next non-body-parameter LIR row
Parent Source: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Trace, publish, verify, and hand off exactly one next valid current-LIR
semantic row outside the already accepted receiver surface after accepted 734
Step 7.50.

## Why This Exists

Idea 734 remains incomplete after accepted Step 7.50, but it can receive only
structured LIR authority into typed Raw BIR. The latest accepted receiver
packet, commit `72a368b06`, consumed the closed-863 direct zero-argument scalar
floating call-result row. No further 734 receiver row is authorized without a
separately accepted producer-side handoff.

## In Scope

- Inspect current LIR producer and verifier behavior for one non-body-
  parameter family still missing a typed 734 disposition.
- Select exactly one valid current-LIR semantic row with native structured
  authority that can be published and verified without presentation recovery.
- Add only the LIR producer/schema/verifier fields and checks required for the
  selected row.
- Verify the row's native identity, current-function or module ownership as
  applicable, typed `LirTypeRef` or other structured type facts, and explicit
  role.
- Add focused malformed-authority coverage for absent, invalid, duplicate,
  foreign, and owner/type/role-incoherent forms relevant to the selected row.
- Hand off the exact row, fields, proof, and malformed matrix back to 734 for
  one later Raw-BIR receiver packet.

## Out Of Scope

- Raw-BIR containers, builders, views, importer dispatch, verifier, or backend
  receiver tests.
- Reopening accepted 734 direct call-result, function-body parameter,
  DirectPointer, DirectScalar, CFG, PHI, memory/object, or other receiver rows.
- Generic residual sweeps, memory/VA sweeps, aggregate/vector sweeps,
  module/type/global/metadata sweeps, CFG/PHI rewrites, inline-assembly
  template/constraint parsing, target-lowering work, or final 797 convergence.
- Recovering authority from text, names, rendered operands, signatures,
  diagnostics, compatibility mirrors, `monostate`, or testcase shape.
- Reworking older open ideas such as 795 or 796 unless the supervisor
  explicitly chooses one as the selected row's owner and reconciles its stale
  return records first.

## Acceptance Criteria

- Exactly one next valid non-body-parameter row is selected and documented.
- The selected row has native structured LIR authority for its identity/type
  tuple.
- The LIR verifier rejects malformed authority before printing or downstream
  use.
- Focused producer/verifier coverage proves the positive row and nearby
  malformed matrix.
- The handoff names the exact 734 return action and states which families
  remain unsupported or separately scoped.
- Supervisor-selected focused proof and any required matching regression guard
  are accepted.

## Reviewer Reject Signals

- Reject any Raw-BIR/importer/container/verifier receiver edit; that work
  belongs to 734 after this handoff closes.
- Reject selecting a function-body parameter row, multiple rows, a generic
  residual sweep, or any row already accepted by 734 through Step 7.50.
- Reject selecting fixed direct-call argument 0 or argument 1 parameter
  authority. Idea 734 already accepted those receiver rows in Steps 7.40 and
  7.41.
- Reject recovering identity, role, type, opcode, or operand relation from
  rendered text, names, signatures, diagnostics,
  compatibility mirrors, `monostate`, or testcase-specific shape.
- Reject expectation downgrades, unsupported-to-supported label changes,
  helper renames, or classification-only edits claimed as authority
  publication.
- Reject broad LIR schema churn, target-lowering behavior, final convergence,
  or unrelated memory/VA, aggregate/vector, module/type/global/metadata, CFG/
  PHI, instruction/terminator, or inline-assembly work.

## Step 1 Handoff: direct one-double-argument scalar floating call-result authority

Selected row: `LirCallOp.direct_one_double_arg_scalar_floating_call_authority`
for exactly a direct, nonvariadic `double(double)` call result.

Acceptance-history justification: this row is not already accepted by 734
through Step 7.50. The accepted body-parameter receiver surface through Steps
7.34-7.49 covers function-body parameter-use rows only. Steps 7.40 and 7.41
specifically cover fixed direct-call argument 0 and argument 1 parameter
authority, not call-result authority. Step 7.50 covers the closed-863 direct
zero-argument scalar floating call-result row only. The selected row has one
fixed `double` argument and therefore is not the Step 7.50 zero-argument row.

Native LIR tuple:

- carrier: `LirDirectOneDoubleArgScalarFloatingCallAuthority`
- result identity: `result` equals the call result `LirValueId`
- owner: `owner` equals the current `LirFunction.link_name_id`
- callee: `callee` equals `LirCallOp.direct_callee_link_name_id`
- return type: `return_type == double`
- argument type: `argument_type == double`
- role: `DirectCallResult`
- downstream consumers: not part of this row. The producer publishes the
  carrier for direct nonvariadic `double(double)` call results even when the
  result has no selected floating binary LHS consumer.

Producer/schema/verifier disposition:

- `src/codegen/lir/ir.hpp` adds the one-row carrier and role enum without
  changing the accepted zero-argument carrier.
- `src/codegen/lir/hir_to_lir/call/target.cpp` publishes the carrier only for
  direct nonvariadic `double(double)` calls with one argument and a structured
  direct callee signature.
- `src/codegen/lir/verify.cpp` rejects absent carriers on selected
  `double(double)` calls, invalid/stale result IDs, foreign owners/callees,
  non-`double(double)` type authority, invalid role, nonmatching direct callee
  signatures, and duplicate results.

Malformed matrix covered in
`tests/frontend/frontend_lir_call_type_ref_test.cpp`:

- absent carrier
- invalid result ID
- stale carrier result
- duplicate result ID
- foreign callee
- foreign owner
- signature/type mismatch
- carrier argument-type mismatch
- invalid role
- producer presence with no selected downstream floating binary LHS consumer
- misleading presentation strings remain non-authority

Proof: fresh focused command
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`
passed with `frontend_lir_call_type_ref` 1/1.

Exact 734 return action: reactivate 734 for one bounded receiver packet only:
receive `LirCallOp.direct_one_double_arg_scalar_floating_call_authority` for the
direct nonvariadic `double(double)` call-result row. Preserve only the native
result `LirValueId`, current-function owner, direct callee `LinkNameId`,
`double(double)` type tuple, and explicit call-result role. Do not require or
infer selected downstream floating binary LHS consumer coherence for this row.
Do not repeat Step 7.50, reopen fixed direct-call arguments 0/1, receive any
body-parameter row, or generalize to other call-result, memory/VA,
aggregate/vector, module/type/global/metadata, CFG/PHI, inline-assembly, or
instruction/terminator families.

## Closure

Close accepted as capability complete for this bounded handoff. Implementation
commit `874499489` selected and published exactly one native producer-side
authority row,
`LirCallOp.direct_one_double_arg_scalar_floating_call_authority`, for direct
nonvariadic `double(double)` call results. The accepted focused before/after
proof
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_before.log/test_after.log 2>&1`
passed 1/1 before and after, `git diff --check` passed, and the broader
frontend proof
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_'; } > /tmp/c4c_frontend_after_864.log 2>&1`
passed 13/13.

No Raw-BIR receiver work landed in this idea. The successor is parent idea 734,
reactivated for exactly one receiver packet for the selected `double(double)`
direct call-result authority row.
