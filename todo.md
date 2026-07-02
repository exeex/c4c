Status: Active
Source Idea Path: ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Bootstrap Global Data-Shape Boundary

# Current Packet

## Just Finished

Activated `ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md`
into `plan.md` and initialized this canonical execution state.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the current bootstrap/global
data-shape handoff boundary with `src/strlen-2.c` or a current stronger
representative plus at least one scalar/global-data shape representative when
available.

## Watchouts

- Keep this route in BIR bootstrap/global data-shape handoff ownership.
- Do not fold prepared global-data layout, RV64 object-route global lowering,
  stack-frame support, move-bundle classification, or F128 work into this
  source idea without a lifecycle split.
- Do not count these related rows as exact `semantic lir_to_bir` producer rows.

## Proof

- Lifecycle activation only; no build or runtime proof required.
