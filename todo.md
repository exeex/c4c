Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect RV64 Emission And Object API Seams

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1.

## Suggested Next

Start Step 1 by inspecting RV64 backend emission, the shared object API, and
existing RV64 backend tests. Record the selected object-emission seam, minimal
smoke subset, required relocation kinds, and focused proof command here before
implementation.

## Watchouts

- Do not route RV64 object output through printed assembly text.
- Keep RV64 relocation kinds and `pcrel_hi` / `pcrel_lo` pairing target-owned.
- Preserve existing RV64 asm-route coverage beside new object-route proof.
- Do not expand into AArch64, CLI exposure, c-testsuite default switching, or
  textual assembler work in this child.

## Proof

Lifecycle-only activation; no build required.
