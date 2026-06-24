# Current Packet

Status: Active
Source Idea Path: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect AArch64 Emission And Object API Seams

## Just Finished

- Lifecycle activation created this `todo.md` for Step 1 of `plan.md`.

## Suggested Next

- Inspect AArch64 emission and object-model seams, then record the chosen
  target-owned implementation seam and first proof command here before code
  changes.

## Watchouts

- Keep compiler object output independent from printed `.s` text.
- Keep RV64 relocation concepts out of AArch64 APIs.
- Do not expose `--codegen obj`, change defaults, or weaken existing asm-route
  coverage in this child.

## Proof

- Lifecycle-only activation; no build required.
