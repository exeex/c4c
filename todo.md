Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Value Materialization Ownership

# Current Packet

## Just Finished

Step 2 - Extract Value Materialization Ownership inspection confirmed Step 2 is
exhausted after the GOT load and scalar multiply materialization extractions.

Concise reasoning tied to the current files:

- `dispatch_value_materialization.hpp/.cpp` now own the explicit value
  materialization surfaces that were small local Step 2 targets:
  `make_load_global_got_materialization_instruction` and
  `lower_scalar_mul_with_distinct_rhs_scratch`, alongside the existing scalar
  cast, FP binary, stack-homed pointer load, local-slot address, and value
  publication helpers.
- `dispatch.cpp` still routes `LoadGlobalInst` and the fallback `Mul` path to
  the value-materialization owner, but no longer owns those helper bodies.
- Remaining dispatch-local materialization-shaped candidates are not small
  Step 2 value-materialization extractions: `lower_store_local_with_address_materialization`
  and `lower_scalar_with_address_materialization` coordinate address
  materialization with memory/scalar dispatch; `lower_missing_*publication`
  helpers are branch/compare publication and producer lookup work; same-block
  producer/cache helpers belong to producer lookup; call-boundary materialized
  address handling belongs to the blocked call-source area.

## Suggested Next

Step 2 is exhausted. Suggested next lifecycle direction is Step 3
publication/producer lookup.

## Watchouts

Do not fold address-materialization orchestration or call-boundary
materialized-address handling into Step 2. The next coherent work should target
publication/producer lookup ownership without touching the blocked call-source
areas.

## Proof

Inspection-only packet; no build/tests run and no logs touched.
