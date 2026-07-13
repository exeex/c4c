# Prepared BIR Planners

Status: scaffold (unimplemented); architecture candidate pending independent
review.

Preparation consumes immutable `VerifiedPreparationInput`, a verified target
layout, and the exact Canonical-BIR/target-context keys defined by the
authoritative schema checkpoint. Each planner publishes typed immutable facts
bound to the complete Canonical revision and target-layout version. No planner
writes facts back into canonical storage or copies the instruction graph.

Order: `abi -> calls -> variadic -> address -> inline_asm -> runtime_helpers`.
The order is preparation-fact order, not mutation order. Shared BIR allocation
consumes the verified cumulative facts after `runtime_helpers`; normal
allocation, eviction, and capacity spill/reload do not belong to MIR. Physical
frame offsets and concrete instruction/register selection remain downstream.

Publication is all-or-nothing. Missing pools, stale revision or target keys,
planner disagreement, incomplete identity bindings, or a planner diagnostic
publishes no cumulative preparation bundle. The later `PreparedBir` name is
reserved for the verified capability over the allocated revision; preparation
facts alone are not `PreparedBir`.

Legacy coverage includes `prepared_fact_boundary.hpp`,
`prepared_object_traversal.*`, `prepared_lookups.*`, and `lookup_agreement.*`.
Their useful contracts must become typed plan APIs; traversal coordinates and
agreement side tables do not survive as authorities.
