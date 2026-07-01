Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Selected Object-Data Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: rebuild the selected object-data evidence for
`src/20000412-1.c`, confirm the prepared object-data facts, and identify the
current RV64 object-route rejection point.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  selected object-data facts.
- Do not infer object labels, extents, initialized bytes, zero-fill,
  relocations, section placement, or symbol identity from testcase names,
  source shape, raw globals, or magic constants.
- Keep static-local object-label publication, parameter-home publication,
  unrelated global access widths, GOT-required globals, thread-local storage,
  F128, and broad ABI work out of this plan.

## Proof

- Pending executor proof for Step 1.
