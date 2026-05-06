Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C dirty repair review blocked acceptance of the current route. The
reviewer report `review/step4c_repair_route_review.md` judged the repair
`drifting from source idea` because HIR rebuilt nested record/template carriers
from `TemplateArgRef::debug_text`, `@origin:args`, rendered module names, and
`module_->struct_defs.find(rendered_name)` recovery.

## Suggested Next

Route the next Step 4C executor packet as a narrowed repair:

- Preserve structured nested template argument carriers through parser-to-HIR
  lowering.
- Carry `tpl_struct_origin_key` and nested `TemplateArgRef` metadata as typed
  data instead of serializing to display text and reparsing it later.
- Keep the directionally aligned pieces only if they no longer depend on
  `debug_text` or rendered-name recovery: `tpl_struct_origin_key` realization,
  recursive nested template struct argument realization, and constructor
  registration for parser-emitted template struct instances.
- Treat codegen aggregate-store changes as acceptable only if the executor can
  prove they are direct fallout from the structured HIR carrier handoff;
  otherwise split or leave them for supervisor routing.
- If the executor cannot preserve these carriers with a bounded Step 4C handoff,
  stop and ask the supervisor to split downstream HIR carrier work into a new
  `ideas/open/` initiative with reviewer reject signals.

## Watchouts

`resolve_record_type_spec` still preserves a parser-local compatibility bridge
for non-layout probes and declaration checks; the stricter helper is private to
constant layout. Do not route `sizeof`/`alignof`/`offsetof` back through the
public compatibility bridge for structured carriers.

Do not accept a repair whose semantic identity path reparses
`TemplateArgRef::debug_text`, `@origin:args`, rendered template instance names,
or rendered module record names. Those strings may remain diagnostics/display
metadata only.

Do not use fixture-name matching, expectation downgrades, or supported-path
weakening to claim Step 4C progress. The known blocker is nested template field
carriers like `Holder::boxed_pair` / `Holder::paired_box` lowering through
`Box_T_void` / `Pair_T_void` or `struct<?>`; the repair must address that
carrier handoff semantically.

## Proof

No acceptance proof for the dirty Step 4C repair. The previous narrow proof is
not sufficient because the reviewer rejected the route. The next executor
should rerun the supervisor-selected Step 4C subset after replacing the
debug-text/rendered-name recovery path with structured carrier preservation.
