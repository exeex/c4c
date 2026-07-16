# LIR CFG And PHI Raw-Binding Evidence

This directory answers the `cfg.phi-raw-bindings-evidence` routing key from
the accepted 812/813 evidence handoff.

- [01_cfg_phi_raw_binding_route.md](01_cfg_phi_raw_binding_route.md)

## Summary

The current modeled `LirPhi` and terminator surface carries native block,
value, type, predecessor, and edge occurrence authority for PHI, direct branch,
conditional branch, switch, return, indirect branch, and unreachable forms.
Verifier and Raw-BIR importer checks reject malformed, foreign, duplicate, and
mismatched edge/value relations, and accepted 734 receiver tests cover the
bounded Raw-BIR rows.

This evidence does not authorize a new 734 receiver handoff. If a future raw
CFG/PHI seam is found, a separate producer/verifier owner must first publish an
exact handoff before 734 receives it. 797 remains downstream of that
disposition.
