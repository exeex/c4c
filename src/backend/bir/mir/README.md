# BIR-to-MIR Boundary

Status: scaffold.

This design namespace describes the replacement machine-stage pipeline. It is
kept under the new BIR architecture during design so the handoff can be reviewed
as one system; implementation may later map accepted contracts onto the existing
`src/backend/mir/` target tree.

MIR owns its own identities, machine operations, virtual/physical registers,
frame objects, and target constraints. BIR IDs may survive only as optional
debug/provenance links.

See [`passes/README.md`](passes/README.md) for the ordered MIR pipeline.
MIR-local recomputable facts are described in [`analysis/`](analysis/README.md),
verification in [`verify/`](verify/README.md), and final target handoff in
[`emission/`](emission/README.md).
