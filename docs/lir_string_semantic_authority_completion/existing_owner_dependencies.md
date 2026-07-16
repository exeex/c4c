# 813 Existing Owner Dependencies

## Preserved Owners

813 is a routing and handoff-documentation route. It does not implement code,
activate receiver work, or turn evidence completion into semantic capability.

| Owner | Preserved scope in this packet |
| --- | --- |
| 734 | Receives only exact accepted typed handoffs. Step 2 authorizes no new direct 734 receiver row from 848, 849, or 850. Accepted bounded CFG/PHI rows described by 850 remain historical 734 evidence and are not reopened. |
| 795 | Keeps its bounded body-parameter authority handoff scope. Step 2 creates no parameter successor and does not alter the 829 -> 830 -> 831 -> 836 return chain. |
| 796 | Remains first owner for future singular residual instruction, terminator, or inline-assembly value/type handoffs when a selected family has native facts and malformed-authority proof. 849 does not authorize a combined residual sweep. |
| 797 | Remains terminal convergence owner after every valid current-LIR row has an explicit typed, checked-mirror, intentional-text, completed-evidence, successor-owned, or receiver-disposition route. 847 is deletion-route input to 797, not proof that 797 is complete. |
| 821 | Retains frontend/manual switch selector authority surfaces. 813 Step 2 does not route switch selector work through 847, 850, or 797. |
| 822 | Retains lowering-produced LIR switch selector authority surfaces. 813 Step 2 does not route switch selector work through 847, 850, or 797. |

## Completed Evidence Routes

### `global.policy-identity-evidence`

Closed 848 is evidence-complete and records no direct 734 handoff. Raw BIR
already stores and validates global object identity and policy facts, but LIR
global weak/visibility and const/global policy still depend on `linkage_vis`
and `qualifier` without a complete global-family verifier contract. If a later
route is required, it must publish and verify an exact structured global policy
contract or record an evidence-backed no-change disposition before 734 can
receive anything. 844 may reuse the evidence as non-type context only.

### `instruction.intrinsic-binding-evidence`

Closed 849 is evidence-complete and records no direct 734 handoff. Structured
`LirInlineAsmValueBinding` covers selected ordinary inline-assembly value
bindings, while templates, constraints, and clobbers remain opaque outward
payload. Future singular residual instruction or inline-assembly binding work
returns first to 796; verifier/dispatch/printer migration for already-published
native facts returns first to 846. 734 and 797 remain downstream of those
accepted handoffs.

### `cfg.phi-raw-bindings-evidence`

Closed 850 is evidence-complete and records no new direct 734 handoff. Current
modeled PHI, direct branch, conditional branch, switch, return, indirect
branch, and unreachable forms carry native block/value/type/edge authority
where supported, and accepted 734 bounded receipts already cover the described
Raw-BIR receiver rows. A future raw CFG/PHI seam must first get a separate
producer/verifier owner and exact handoff before 734 receives one bounded row.

## 847 Handoff Relation

Closed 847 supplies terminal deletion-route evidence to 797:

- implicit LIR opcode/operand string conversions and mutable operand string
  access were deleted;
- selected semantic string authority was replaced with typed facts;
- LIR type refs were threaded through the BIR lowering paths selected by 847;
- remaining text bridges in its scope were classified as deliberate no-id,
  legacy, or owner-specific compatibility boundaries.

This relation does not close 797 and does not override 812/813 residual
routing, 821/822 switch selector ownership, or 734 receiver dependencies.

## Dependency Order For Step 3

Step 3 should publish the successor queue and terminal handoff with this order:

1. Record the three stable keys as completed-evidence dispositions with no new
   Step 2 successor.
2. Preserve future 796/846/global-policy producer routes only as conditional
   downstream owners named by 848/849/850, not as newly activated 813 work.
3. Carry 847 as deletion evidence to 797.
4. Keep 797 blocked from final convergence until all producer, receiver,
   intentional-text, checked-mirror, and completed-evidence dispositions are
   represented in the final handoff package.
