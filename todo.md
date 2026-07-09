Status: Active
Source Idea Path: ideas/open/641_aggregate_global_object_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh And Classify Aggregate Global Residuals

# Current Packet

## Just Finished

Lifecycle activation created the idea 641 runbook and initialized execution
state for Step 1.

## Suggested Next

Start Step 1 by refreshing diagnostics for the representative aggregate
global-object residual rows and classifying their first owners before choosing
any implementation family.

## Watchouts

- Do not treat aggregate global-object materialization as scalar direct
  `addr @symbol` local-memory support.
- Do not add testcase-shaped handling for the named rows or specific
  aggregate offsets.
- Do not infer aggregate lanes, byte ranges, or destination authority from
  final assembly layout or source object spelling.

## Proof

Lifecycle-only activation; no build or test proof was run.
