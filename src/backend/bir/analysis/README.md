# Revision-Bound BIR Analyses

Status: scaffold.

Analyses are immutable results keyed by exact module/function revision. Dense
indices exist only inside one result. A mutation summary and declared preserved
analyses control invalidation; stale results are never observable.

Initial analysis families:

- [`cfg`](cfg/README.md)
- [`dominance`](dominance/README.md)
- [`liveness`](liveness/README.md)
- [`memory_effects`](memory_effects/README.md)
- [`provenance`](provenance/README.md)
- [`call_graph`](call_graph/README.md)
- [`comparison`](comparison/README.md)
- [`publication`](publication/README.md)

Def-use is a core editor-maintained invariant, not an optional analysis.
