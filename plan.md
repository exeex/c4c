# AArch64 Minimal Relocatable ELF Object Emission Runbook

Status: Active
Source Idea: ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md
Activated from: ideas/open/329_native_object_emission_umbrella.md
Depends on closed children:
- ideas/closed/330_native_object_model_and_emission_api.md
- ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md

## Purpose

Add the first direct AArch64 relocatable ELF object-emission path for the
current supported backend subset, using the shared native object model and ELF
writer instead of printing assembly and parsing it back.

## Goal

Produce inspectable and linkable AArch64 `.o` output for a minimal real backend
subset while preserving the existing AArch64 asm route as independent coverage.

## Core Rule

AArch64 object emission must be target-owned and structured: AArch64 backend
records produce encoded bytes plus typed AArch64 fixups, AArch64 lowers those
fixups into ELF relocation numbers, and the shared object model/writer
serializes the result. Do not route compiler object output through printed `.s`
text, and do not leak RV64 relocation concepts into AArch64 APIs.

## Read First

- `ideas/open/332_aarch64_minimal_relocatable_elf_object_emission.md`
- `ideas/open/329_native_object_emission_umbrella.md`
- `ideas/closed/330_native_object_model_and_emission_api.md`
- `ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md`
- `src/backend/mir/object/`
- AArch64 backend emission code under `src/backend/mir/aarch64/`
- existing AArch64 backend tests and asm-route proof patterns

## Current Scope

- Add AArch64 target records or helpers for encoded instruction bytes plus
  typed target fixups.
- Integrate AArch64 function/data emission with the shared object model.
- Support the minimal relocation set needed by current AArch64 backend output,
  including calls, branches, symbol references, and ADRP/ADD-style address
  materialization when present in the supported subset.
- Keep AArch64 ELF machine, flags, relocation numbers, instruction encodings,
  fixup meanings, alignment behavior, and pairing rules target-local.
- Add object contract and link/runtime smoke tests beside existing asm-route
  coverage where the local toolchain supports the proof.

## Non-Goals

- Do not implement new RV64 object-emission behavior in this child.
- Do not expose or default `--codegen obj` through the CLI in this child.
- Do not run c-testsuite-wide object-route scans or default switching.
- Do not build a GNU-compatible textual assembler.
- Do not replace or weaken the existing AArch64 `.s` route.
- Do not absorb broad AArch64 backend semantic fixes unrelated to object
  emission.

## Working Model

The direct AArch64 object route should follow this shape:

```text
AArch64 backend records -> AArch64 encoder/fixups -> shared object records -> ELF .o
```

The asm route remains separate:

```text
AArch64 backend records -> asm writer -> .s
```

Target-local code owns AArch64 instruction encoding, relocation enum names,
relocation-number mapping, branch/call fixups, ADRP/ADD-style address
materialization policy, and target ELF metadata. The shared object layer owns
sections, symbols, labels, relocation records, and ELF serialization only after
AArch64 has chosen target relocation numbers.

## Execution Rules

- Prefer small AArch64-owned helpers over growing target logic inside the
  shared object writer.
- Use typed AArch64 fixups or relocation records at target boundaries; avoid
  loose strings or scattered raw relocation numbers in backend emission.
- Keep RV64 `pcrel_hi` / `pcrel_lo` naming and pairing policy out of AArch64
  APIs.
- Add tests for each meaningful target-object slice: structure, relocation
  shape, linkability, and runtime behavior where possible.
- Preserve existing asm-route tests when touching shared AArch64 emission
  surfaces.
- Escalate to a separate idea for unrelated AArch64 semantic failures
  discovered while proving object output.

## Step 1: Inspect AArch64 Emission And Object API Seams

Goal: identify the smallest AArch64 object-emission seam that can reuse the
closed shared object model and the RV64-proven writer path without forking the
current asm writer route.

Primary targets:

- `src/backend/mir/aarch64/`
- `src/backend/mir/object/`
- existing AArch64 backend tests

Actions:

- Inspect current AArch64 function, data, label, symbol, call, branch, and
  address-materialization emission paths.
- Identify where encoded bytes and AArch64 typed fixups should be produced
  without changing asm-route text emission behavior.
- Map the minimal relocation kinds required by the first smoke subset.
- Record the chosen implementation seam, first proof command, and any known
  unsupported AArch64 object features in `todo.md`.

Completion check:

- `todo.md` names the target-owned seam, first minimal smoke subset, required
  relocation kinds, and proof command for the first implementation slice.

## Step 2: Add AArch64 Encoded Fragment And Fixup Records

Goal: represent AArch64 object-emission output as bytes plus typed target
fixups before lowering to target-neutral object records.

Actions:

- Add AArch64-owned records or helpers for encoded instruction/data fragments
  and typed fixups needed by the minimal subset.
- Keep relocation names, instruction patching, branch/call semantics, and
  ADRP/ADD-style address materialization target-local.
- Avoid changing shared object records unless a real API gap from child 330 is
  discovered.
- Add focused tests that prove AArch64 fixups can express call or branch,
  symbol reference, and address materialization cases selected for the subset.

Completion check:

- AArch64 target tests can construct the required typed fixups without printed
  assembly text, and existing AArch64 asm-route tests still pass for touched
  surfaces.

## Step 3: Integrate AArch64 Machine Emission With The Object Model

Goal: build a shared `ObjectModule` from AArch64 backend output for the
supported minimal subset.

Actions:

- Emit `.text`, `.data`, and `.bss` contributions through shared object
  helpers where supported by current AArch64 backend output.
- Publish function/data symbols and section-local labels with stable
  identities.
- Attach AArch64-lowered relocation records to the correct sections and
  offsets.
- Keep unsupported AArch64 object features explicit and diagnosed in tests or
  executor notes instead of silently falling back to asm.

Completion check:

- A focused AArch64 object-module test proves real target-like emission
  produces sections, symbols, labels, and relocations through shared object
  helpers.

## Step 4: Implement AArch64 Relocatable ELF Object Output For A Smoke Subset

Goal: serialize minimal AArch64 backend output into structurally valid,
relocatable, linkable ELF objects.

Actions:

- Supply AArch64 ELF machine, flags, and relocation-number mapping to the
  shared ELF writer.
- Prove at least one external symbol, call, or branch relocation.
- Prove at least one symbol-address relocation needed by the current supported
  subset, such as ADRP/ADD-style address materialization when present.
- Add readelf/objdump-style structural assertions where available.
- Add a link/runtime smoke test for the supported subset when the local
  toolchain can link the object.

Completion check:

- Focused AArch64 object tests inspect the produced `.o`, prove relocation
  shape, and link/run the claimed smoke subset without using printed assembly
  as an intermediate compiler artifact.

## Step 5: Preserve AArch64 Asm Route And Validate Handoff

Goal: finish the child with enough proof for later CLI/test integration to
expose AArch64 object output deliberately.

Actions:

- Run a fresh build plus focused AArch64 object tests.
- Run equivalent or nearby AArch64 asm-route tests for touched emission
  surfaces.
- Update `todo.md` with proof commands, limitations, and the handoff notes for
  the CLI/test integration child.
- Do not add user-visible `--codegen obj` default behavior in this child.

Completion check:

- AArch64 direct object emission satisfies the source idea acceptance
  expectations for the minimal supported subset, equivalent asm-route coverage
  remains meaningful, and the supervisor can ask the plan owner to close child
  332 or hand off to later children.
