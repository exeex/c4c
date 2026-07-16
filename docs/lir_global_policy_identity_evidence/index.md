# LIR Global Policy And Symbol-Identity Evidence

This directory answers the `global.policy-identity-evidence` routing key from
the accepted 812/813 evidence handoff.

- [01_global_policy_identity_route.md](01_global_policy_identity_route.md)

## Summary

Raw BIR already has explicit storage and receiver checks for global object
identity, internal/weak/const/extern flags, visibility, alignment, and ordered
initializer function links. The importer rejects malformed policy combinations
and builder-level link-name mismatches.

The missing seam is earlier: LIR still carries weak/visibility policy in
`LirGlobal.linkage_vis` and const/global policy in `LirGlobal.qualifier`, and
the LIR verifier does not currently enforce a complete global policy family
contract for those fields. Therefore this evidence does not authorize a 734
Raw-BIR receiver handoff by itself.

Return relation:

- 844 can reuse this as non-type global policy evidence outside its closed
  type-fact route.
- 734 remains downstream until a later owner publishes and verifies an exact
  structured global policy/symbol-identity handoff, or records an explicit
  evidence-backed no-change disposition.
- 797 must wait for that 734 disposition before final convergence.
