Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Static Eval Callers

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for
`plan.md` Step 1.

## Suggested Next

Inventory all `static_eval_int` declarations, definitions, wrappers, and
callers. Classify each caller as structured-metadata capable, `TextId` capable,
rendered-compatibility only, or unrelated to enum constants. Record the caller
inventory here before implementation or test edits.

## Watchouts

- Do not treat raw `TextId` alone as sufficient when enum/domain scope matters.
- Do not let complete structured misses fall back to `enum_consts.find(n->name)`.
- Keep rendered string lookup only as an explicit no-metadata compatibility
  bridge.
- Do not reopen the full consteval interpreter or validate table ownership.

## Proof

Lifecycle-only activation; no build or test proof required yet.
