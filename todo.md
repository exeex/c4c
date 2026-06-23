Status: Active
Source Idea Path: ideas/open/320_rv64_nested_select_chain_store_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Nested Store-Source Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00144.c`, inspecting the prepared
store-source/select-chain records around the later ternary joins, and
classifying the first nested destination-access publication failure.

## Watchouts

- Do not special-case `src/00144.c`, `tern.end.*`, fixed source ternary shapes,
  or observed RV64 instruction text.
- Do not treat already-repaired simple pointer-typed select publication as
  completion for this nested store-source route.
- Do not fold idea 321's i16 local-array/select-store residual, stack-homed
  fused compare control flow, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Lifecycle-only activation. No build or tests were run.
