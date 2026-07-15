# LIR Next Function-Body Parameter Authority Handoff

Status: Closed
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.37
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish one next valid function-body parameter-use semantic row as
a native structured LIR authority contract, so 734 can later receive exactly
that row without presentation-derived recovery.

## Why This Exists

734 accepted only DirectScalar parameter uses in a direct pointer GEP, binary
LHS, binary RHS, and exact return-value role. Its no-omission source contract
still includes other function-body parameter forms, but no additional row is
currently authorized for Raw-BIR receipt. The producer/schema/verifier first
owner must identify one native row before a receiver can proceed.

## In Scope

- Trace the next candidate function-body parameter-use route and select one
  semantic row only when its value identity, current-function ownership,
  parameter index, type, ABI classification, role, and exact consuming
  operand relation have native structured representation.
- Publish and verifier-check only that selected row, including malformed,
  missing, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent rejection as applicable.
- Add focused same-feature producer positive/negative coverage and write an
  exact one-row receiver handoff for 734.

## Out Of Scope

- Raw-BIR containers, importer or receiver/verifier edits, broad parameter
  admission, declaration facts treated as body-use authority, ABI conversion,
  or any second parameter row.
- Reopening accepted direct-pointer, DirectScalar binary-LHS, binary-RHS, or
  ReturnValue rows; memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, target lowering, or text
  parsing.

## Acceptance Criteria

- Exactly one selected body-parameter semantic row has a native checked
  structured contract and focused nearby positive/negative proof.
- The handoff identifies its exact tuple, consumer relation, failure cases,
  and the one bounded 734 receiver return action; all nonselected forms stay
  fail closed.
- No Raw-BIR/importer code changes, presentation-derived authority, or
  expectation weakening is used to claim the handoff.

## Step 1 Selection Record

The sole selected row is an unchanged current-function integer parameter with
`LirNativeBodyParameterAbi::DirectScalar`, consumed directly as
`LirSwitch.selector`. Its required native tuple is the parameter-definition
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
`LirTypeRef`, `DirectScalar` ABI, a new `SwitchSelector` role, and the exact
consumer relation that `LirSwitch.selector` equals that value and
`LirSwitch.selector_type_ref` equals that type.

The first producer is `StmtEmitter::emit_control_flow_stmt(const SwitchStmt&)`;
the publication/verifier seam is a dedicated optional `LirSwitch` selector
authority checked by `verify_switch_selector`. It must not reuse the accepted
`LirBinOp.scalar_lhs_parameter_authority` or its materializing add: that is a
binary-LHS relation, not this direct switch-selector row. DirectPointer,
DirectScalar binary-LHS, binary-RHS, ReturnValue, and every other parameter
form remain fail closed.

## Step 3: 734 Receiver Handoff

The sole receiver-authorized row for 734 is:

| value | owner | parameter index | type | ABI | role |
| --- | --- | --- | --- | --- | --- |
| native parameter-definition `LirValueId` | current `LirFunction.link_name_id` | native parameter-definition index | native parameter-definition `LirTypeRef` | `LirNativeBodyParameterAbi::DirectScalar` | `LirSwitchSelectorParameterRole::SwitchSelector` |

It is present only through `LirSwitch.selector_parameter_authority` for an
unchanged current-function direct-scalar integer parameter. Its checked
consumer relation is exact: `LirSwitch.selector == authority.value` and
`LirSwitch.selector_type_ref == authority.type`. The verifier rejects a
missing authority for this native route and rejects invalid, duplicate,
foreign, owner/index/type/ABI/role-mismatched, or consumer-incoherent
authority. It does not derive the row from rendered parameter or selector
text.

734 has exactly one bounded return action: add one receiver mapping that
consumes this `LirSwitch.selector_parameter_authority` tuple for the direct
`LirSwitch.selector` use, preserving the two stated equality checks. It must
not admit a second parameter row, recover facts from presentation, reuse
binary-LHS authority, or materialize an `add`. DirectPointer, DirectScalar
binary-LHS, binary-RHS, ReturnValue, and all other nonselected forms remain
fail closed.

## Closure Record

Status: capability complete.

Step 2 published the dedicated native `LirSwitch` selector authority and
focused positive/malformed coverage in `37674ad76` (`lir: publish direct
scalar switch selector authority`). The supervisor accepted a fresh
`cmake --build --preset default`, the exact focused
`ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`,
and matching regression guard from baseline 0/1 expected abort to after 1/1
pass. Step 3 above is the durable one-row 734 handoff. No Raw-BIR/importer or
receiver code changed in this idea.

## Reviewer Reject Signals

- Reject selection from rendered parameter names, types, signatures, operand
  spelling, printer output, or testcase shape rather than native authority.
- Reject generic parameter admission, broad ABI conversion, multiple rows,
  receiver edits, expectation downgrades, or verifier weakening claimed as
  this one-row handoff.
- Reject retaining a missing/ambiguous/foreign/consumer-incoherent authority
  path behind a renamed carrier or leaving selected malformed input accepted.
- Reject any reopening of the four accepted 734 parameter receiver rows or
  expansion into non-parameter semantic families.
