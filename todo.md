Status: Active
Source Idea Path: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Review Final Module Assembly Boundary

# Current Packet

## Just Finished

Step 4 reviewed final object assembly, ELF config, section ordering, section
flags, relocation attachment, public entrypoints, and result construction.

Moved helpers: none. No narrower final-module helper was extracted.

Explicit dependencies kept visible: `build_rv64_prepared_text_object_module`
still exposes the ordered sequence from prepared function admission through
`RiscvObjectFunction` collection, `.text` module construction, data/rodata/bss
attachment, and module-result diagnostics. `write_rv64_relocatable_elf_object`
still exposes the `rv64_relocatable_elf_config` handoff to the object writer,
including ELF64, little-endian, RISC-V machine id, and RV64 double-float ABI
flags. `write_rv64_prepared_relocatable_elf_object_with_diagnostics` still owns
the public module-to-image sequencing and image-result diagnostics.

Parked helpers: final module assembly remains central. Moving it now would hide
`.text`, `.rodata`, `.data`, `.bss`, section ordering, relocation ownership,
ELF flags, and public entrypoint/result sequencing behind a broad wrapper rather
than exposing a smaller contract.

## Suggested Next

Execute Step 5 by reviewing close readiness for idea 543. Confirm the Step 2
and Step 4 no-code decisions plus the Step 3 text-side helper extraction satisfy
the runbook without changing object bytes, relocations, section layout, ELF
flags, public entrypoints, or expectations.

## Watchouts

- Preserve object bytes, relocations, symbol bindings, section layout, ELF
  flags, public entrypoints, and zero-fill behavior.
- Keep text fixup attachment separate from data-object pointer relocation
  handling; Step 3 intentionally split only the text-side helper path.
- Step 4 should not move final module assembly if doing so would hide `.text`,
  `.rodata`, `.data`, `.bss`, relocation, section ordering, or ELF flag
  ownership behind a broad context.
- Do not touch tests, expectations, unsupported markers, or runtime contracts.
- Keep `test_baseline.new.log` treated as a rejected full-suite candidate, not
  an accepted baseline.
- Leave `review/global_address_helper_cleanup_review.md` untouched.

## Proof

No code changes were made for Step 4, so no `test_after.log` was written and no
test subset was run. Local formatting check only: `git diff --check -- todo.md`.
