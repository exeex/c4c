Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Dispatch Materialization Fallbacks

# Current Packet

## Just Finished

Activation only. No executor packet has run for this plan yet.

## Suggested Next

Start Step 1 - Audit Dispatch Materialization Fallbacks. Classify the duplicate
authority fallback paths in `dispatch_value_materialization.cpp`, identify the
first bounded repair target, and record the prepared fact or missing shared
query that should own the semantic authority.

## Watchouts

- Do not deepen same-block producer recursion or add named-case shortcuts.
- Do not replace prepared block-entry/value-home checks with raw move-bundle or
  value-name scans.
- Do not hard-code global-name spelling, GOT policy, or direct-global
  select-chain shortcuts.
- Keep `globals.cpp` and `fp_value_materialization.cpp` out of this plan; they
  belong to source idea `54`.
- Keep routine executor progress in this file rather than rewriting `plan.md`.

## Proof

Lifecycle activation only; no build or test proof required.
