# RV64 Minimal Relocatable ELF Object Emission Runbook

Status: Active
Source Idea: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Activated from: ideas/open/329_native_object_emission_umbrella.md
Depends on closed child: ideas/closed/330_native_object_model_and_emission_api.md

## Purpose

Add the first direct RV64 relocatable ELF object-emission path for the current
supported backend subset, using the shared native object model and ELF writer
instead of printing assembly and parsing it back.

## Goal

Produce linkable RV64 `.o` output for a minimal real backend subset while
preserving the existing RV64 asm route as independent coverage.

## Core Rule

RV64 object emission must be target-owned and structured: machine records
produce encoded bytes plus typed RV64 fixups, RV64 lowers those fixups into ELF
relocation numbers, and the shared object model/writer serializes the result.
Do not route compiler object output through printed `.s` text.

## Read First

- `ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md`
- `ideas/open/329_native_object_emission_umbrella.md`
- `ideas/closed/330_native_object_model_and_emission_api.md`
- `src/backend/mir/object/`
- RV64 backend emission code under `src/backend/mir/riscv/`
- existing RV64 backend tests and asm-route proof patterns

## Current Scope

- Add RV64 target records or helpers for encoded instruction bytes plus typed
  fixups.
- Integrate RV64 function/data emission with the shared object model.
- Support the minimal relocation set needed by current RV64 backend output,
  including calls, branches or jumps, symbol references, and PC-relative
  address materialization used by supported tests.
- Implement correct `pcrel_hi` / `pcrel_lo` pairing, including synthetic
  AUIPC-site labels for low relocations when required.
- Add object contract and link/runtime smoke tests beside existing asm-route
  coverage.

## Non-Goals

- Do not implement AArch64 object emission.
- Do not expose or default `--codegen obj` through the CLI in this child.
- Do not run c-testsuite-wide object-route scans or default switching.
- Do not build a GNU-compatible textual assembler.
- Do not replace or weaken the existing RV64 `.s` route.
- Do not absorb broad RV64 backend semantic fixes unrelated to object emission.

## Working Model

The direct RV64 object route should follow this shape:

```text
RV64 backend records -> RV64 encoder/fixups -> shared object records -> ELF .o
```

The asm route remains separate:

```text
RV64 backend records -> asm writer -> .s
```

Target-local code owns RV64 instruction encoding, relocation enum names,
relocation-number mapping, and any `pcrel_hi` / `pcrel_lo` pairing policy. The
shared object layer owns sections, symbols, labels, relocation records, and
ELF serialization only after RV64 has chosen the target relocation numbers.

## Execution Rules

- Prefer small target-owned helpers over growing target logic inside the shared
  object writer.
- Use typed RV64 fixups or relocation records at target boundaries; avoid loose
  strings or scattered raw relocation numbers in backend emission.
- Keep pcrel-low relocations tied to the correct AUIPC-site synthetic label
  when RV64 relocation semantics require it.
- Add tests for each meaningful target-object slice: structure, relocation
  shape, linkability, and runtime behavior where possible.
- Preserve existing asm-route tests when touching shared RV64 emission
  surfaces.
- Escalate to a separate idea for unrelated RV64 semantic failures discovered
  while proving object output.

## Step 1: Inspect RV64 Emission And Object API Seams

Goal: identify the smallest RV64 object-emission seam that can reuse the
closed shared object model without forking the current asm writer route.

Primary targets:

- `src/backend/mir/riscv/`
- `src/backend/mir/object/`
- existing RV64 backend tests

Actions:

- Inspect current RV64 function, data, label, symbol, call, branch, and
  address-materialization emission paths.
- Identify where encoded bytes and RV64 typed fixups should be produced
  without changing asm-route text emission behavior.
- Map the minimal relocation kinds required by the first smoke subset.
- Record the chosen implementation seam, first proof command, and any known
  unsupported RV64 object features in `todo.md`.

Completion check:

- `todo.md` names the target-owned seam, first minimal smoke subset, required
  relocation kinds, and proof command for the first implementation slice.

## Step 2: Add RV64 Encoded Fragment And Fixup Records

Goal: represent RV64 object-emission output as bytes plus typed target fixups
before lowering to target-neutral object records.

Actions:

- Add RV64-owned records or helpers for encoded instruction/data fragments and
  typed fixups needed by the minimal subset.
- Keep relocation names and pairing policy target-local.
- Avoid changing shared object records unless a real API gap from child 330 is
  discovered.
- Add focused tests that prove RV64 fixups can express call, branch or jump,
  symbol reference, and PC-relative address materialization cases selected for
  the subset.

Completion check:

- RV64 target tests can construct the required typed fixups without printed
  assembly text, and existing RV64 asm-route tests still pass for touched
  surfaces.

## Step 3: Integrate RV64 Machine Emission With The Object Model

Goal: build a shared `ObjectModule` from RV64 backend output for the supported
minimal subset.

Actions:

- Emit `.text`, `.data`, and `.bss` contributions through shared object
  helpers where supported by current RV64 backend output.
- Publish function/data symbols and section-local labels with stable
  identities.
- Attach RV64-lowered relocation records to the correct sections and offsets.
- Keep unsupported RV64 object features explicit and diagnosed in tests or
  executor notes instead of silently falling back to asm.

Completion check:

- A focused RV64 object-module test proves real target-like emission produces
  sections, symbols, labels, and relocations through shared object helpers.

## Step 4: Implement RV64 Relocatable ELF Object Output For A Smoke Subset

Goal: serialize minimal RV64 backend output into structurally valid,
relocatable, linkable ELF objects.

Actions:

- Supply RV64 ELF machine, flags, and relocation-number mapping to the shared
  ELF writer.
- Prove at least one external symbol or call relocation.
- Prove required `pcrel_hi` / `pcrel_lo` pairing, including AUIPC-site labels
  for low relocations when required by the selected subset.
- Add readelf/objdump-style structural assertions where available.
- Add a link/runtime smoke test for the supported subset when the local toolchain
  can link the object.

Completion check:

- Focused RV64 object tests inspect the produced `.o`, prove relocation shape,
  and link/run the claimed smoke subset without using printed assembly as an
  intermediate compiler artifact.

## Step 5: Preserve RV64 Asm Route And Validate Handoff

Goal: finish the child with enough proof for later CLI/test integration to
expose RV64 object output deliberately.

Actions:

- Run a fresh build plus focused RV64 object tests.
- Run equivalent or nearby RV64 asm-route tests for touched emission surfaces.
- Update `todo.md` with proof commands, limitations, and the handoff notes for
  the CLI/test integration child.
- Do not add user-visible `--codegen obj` default behavior in this child.

Completion check:

- RV64 direct object emission satisfies the source idea acceptance expectations
  for the minimal supported subset, equivalent asm-route coverage remains
  meaningful, and the supervisor can ask the plan owner to close child 331 or
  hand off to later children.
