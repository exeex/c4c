# Active Plan Todo

Status: Active
Source Idea: ideas/open/48_template_trait_normalization_and_owner_resolution.md
Source Plan: plan.md

## Active Item

- Step 1: Stabilize alias-template argument mapping
- Current slice: extend Step 1 coverage from concrete-type alias packs to
  parser-side alias argument-ref preservation for transformed member-typedef
  outputs, including dependent reference-qualified alias-of-alias cases that
  must defer owner member resolution instead of collapsing to the primary
  template too early

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
- Preserved substituted deferred-template arg refs without empty-pack
  placeholder slots and routed alias member-typedef owner resolution through
  transformed owner args when explicit owner metadata survives parsing.
- Added focused runtime coverage for a dependent alias-of-alias
  `remove_ref_t<B&>` case that should strip the reference only after template
  substitution, and updated alias member handling to preserve full deferred
  owner arg refs for dependent member-typedef lookups.

## Notes

- Start from the suggested alias-template repro set in the source idea and keep
  the first implementation slice as small as possible.
- First targeted coverage should exercise alias expansion where one alias
  parameter is a type pack and the aliased target needs the expanded pack
  preserved, not flattened to a single positional binding.
- This iteration is focused on alias member-typedef resolution using the
  transformed target arguments rather than the original alias call-site
  arguments.
- A narrower blocker was confirmed while probing reordered/defaulted
  member-typedef aliases: `parse_dependent_typename_specifier()` currently
  collapses dependent `Owner<...>::type` spellings to the primary template's
  member typedef (for example `Head`) at alias registration time, so the owner
  and transformed argument refs are lost before alias substitution runs.
- Landed prep work in `parser_types_base.cpp` keeps transformed owner arg refs
  aligned with substitution and avoids reintroducing empty-pack placeholder
  slots in deferred template arg refs.
- The next alias follow-up, if still needed after this slice, is the narrower
  parser registration issue where dependent `typename Owner<...>::type`
  spellings can still lose owner context before alias substitution runs.
- A separate pack-specialization issue still exists for direct variadic class
  template specialization selection; keep that out of this Step 1 alias slice
  unless it becomes required for the next targeted alias regression.
- Do not broaden into unrelated template machinery unless it blocks the active
  slice.
- If a separate initiative appears during execution, record it under
  `ideas/open/` instead of mutating this plan.
