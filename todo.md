# Active Plan Todo

Status: Active
Source Idea: ideas/open/48_template_trait_normalization_and_owner_resolution.md
Source Plan: plan.md

## Active Item

- Step 1: Stabilize alias-template argument mapping
- Current slice: extend Step 1 coverage from concrete-type alias packs to
  defaulted or reordered alias-template pack applications without widening
  into broader template-pack specialization work

## Pending Items

- Step 2: Generalize trait and variable-template normalization
- Step 3: Complete scoped-enum semantics
- Step 4: Replace heuristic owner recovery

## Completed Items

- Activated the source idea into `plan.md`
- Added runtime coverage proving a non-empty alias-template type pack still
  resolves to its concrete aliased type.
- Fixed template-argument disambiguation so known alias-template type heads are
  parsed as type arguments instead of being forced down the value-expression
  path.
- Tightened alias-template substitution bookkeeping for pack parameters and
  prevented `_t` owner-recovery heuristics from overriding already-concrete
  aliased types.

## Notes

- Start from the suggested alias-template repro set in the source idea and keep
  the first implementation slice as small as possible.
- First targeted coverage should exercise alias expansion where one alias
  parameter is a type pack and the aliased target needs the expanded pack
  preserved, not flattened to a single positional binding.
- A separate pack-specialization issue still exists for direct variadic class
  template specialization selection; keep that out of this Step 1 alias slice
  unless it becomes required for the next targeted alias regression.
- Do not broaden into unrelated template machinery unless it blocks the active
  slice.
- If a separate initiative appears during execution, record it under
  `ideas/open/` instead of mutating this plan.
