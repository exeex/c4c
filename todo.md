Status: Active
Source Idea Path: ideas/open/329_native_object_emission_umbrella.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Create Focused Child Ideas

# Current Packet

## Just Finished

Completed Step 5 by creating the focused native object-emission child queue
under `ideas/open/`:

- `ideas/open/330_native_object_model_and_emission_api.md`
- `ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md`
- `ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md`
- `ideas/open/333_codegen_obj_cli_and_test_integration.md`
- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `ideas/open/335_textual_assembler_object_route_followup.md`

The queue covers the required ownership boundaries from the umbrella research:
shared object model/API first, RV64 minimal object emission, AArch64 minimal
object emission, CLI/test integration for `--codegen obj`, broader object-route
scan and default-readiness, and a scoped textual assembler follow-up. Each child
idea includes a narrow goal, why it exists, in-scope work, out-of-scope work,
acceptance/proof expectations, dependency notes, and concrete reviewer reject
signals.

No implementation files, tests, `plan.md`, or closed ideas were edited.

## Suggested Next

Proceed to Step 6: switch lifecycle state away from the umbrella by
deactivating or replacing the current umbrella runbook and activating
`ideas/open/330_native_object_model_and_emission_api.md` as the first child
idea. Keep `ideas/open/329_native_object_emission_umbrella.md` open until the
child queue closes and final umbrella acceptance review confirms the children
compose into native object-emission readiness.

## Watchouts

- Do not implement backend object emission while the umbrella remains active.
- The first child should establish shared object records and serialization
  surfaces without leaking RV64-specific relocation names into shared APIs.
- Direct compiler `.o` emission must stay independent of printed `.s` parsing.
- RV64 and AArch64 target object children should use target-owned typed fixups
  lowered into shared object relocations.
- Existing asm-route tests must remain present and meaningful throughout the
  object-route work.

## Proof

Research/decomposition-only packet; no build or test validation required and no
new `test_after.log` was written. Proof is the created child source ideas and
this `todo.md` Step 5 queue record.
