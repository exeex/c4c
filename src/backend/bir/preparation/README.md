# Prepared BIR Planners

Status: scaffold.

Preparation consumes immutable, verified `CanonicalBir` plus a target context.
Each planner produces a typed immutable plan tied to exact BIR revisions. No
planner writes facts back into canonical storage.

Order: `abi -> calls -> variadic -> address -> inline_asm -> runtime_helpers`.
Frame layout and register allocation consume MIR and therefore live in the MIR
pipeline, even when a higher-level prepared contract supplies their inputs.

Legacy coverage includes `prepared_fact_boundary.hpp`,
`prepared_object_traversal.*`, `prepared_lookups.*`, and `lookup_agreement.*`.
Their useful contracts must become typed plan APIs; traversal coordinates and
agreement side tables do not survive as authorities.
