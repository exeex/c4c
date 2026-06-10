# Current Packet

Status: Active
Source Idea Path: ideas/open/149_residual_prepared_lookup_include_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map residual prepared lookup includes and narrow owners

## Just Finished

Activation created this executor scratchpad for Step 1. No implementation
packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: map residual `prepared_lookups.hpp` consumers,
classify keep/remove decisions, and identify the narrow replacement headers
before editing code.

## Watchouts

- Do not remove `prepared_lookups.hpp` from files that still name
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Do not replace the old facade with another broad umbrella header.
- Keep the cleanup behavior-preserving and include-focused.

## Proof

Not run. Lifecycle-only activation.
