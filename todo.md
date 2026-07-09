Status: Active
Source Idea Path: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh and classify pointer-loaded-from-global rows

# Current Packet

## Just Finished

Activation created the runbook for idea 639. No execution packet has run yet.

## Suggested Next

Start Step 1 by refreshing and classifying the pointer-loaded-from-global
representative rows with the supervisor-selected narrow proof command.

## Watchouts

- Do not treat `bir.load_global ptr @x` followed by local memory as direct
  `addr @x` local-memory support.
- Keep prepared global value-location, aggregate stack-home, string-constant,
  direct global-symbol, ordinary frame-slot, and runtime-only rows outside this
  policy.
- Do not claim progress from expectations, unsupported markers, allowlists,
  timeouts, accounting, runtime policy, helper renames, or diagnostic wording
  alone.

## Proof

Lifecycle-only activation. No build or runtime proof was run.
