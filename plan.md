# AArch64 Scalar Cast Stack-Homed Register Source Publication Runbook

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md Step 4 split

## Purpose

Repair the post-339 selected scalar cast path where a stack-homed source has a
prepared consumer stack-to-register move, but the AArch64 printer still receives
no structured register source operand.

## Goal

Make selected scalar casts publish and consume the prepared register source for
stack-homed inputs before AArch64 machine printing.

## Core Rule

Fix the scalar cast register-source publication boundary generally. Do not
special-case `00143`, value ids, instruction numbers, source lines, registers,
or diagnostic strings.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- `ideas/closed/338_aarch64_scalar_cast_register_source_operand_facts.md`
- `ideas/closed/339_aarch64_scalar_local_storage_writeback_sizing.md`
- Current evidence preserved in the focused idea and umbrella deactivation
  note:
  - `%t76 = bir.select ... i16`
  - `%t81 = bir.sext i16 %t76 to i32`
  - `%t76 value_id=308 kind=stack_slot`
  - `%t81 value_id=388 kind=register reg=x13`
  - prepared move `from_value_id=308 to_value_id=388`
    `destination_storage=register reason=consumer_stack_to_register`
  - printer diagnostic: scalar `sign_extend` requires a structured register
    source operand

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main --mir-focus-block block_16 tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s`
- Focused backend coverage should exercise scalar casts whose source value is
  stack-homed but has a prepared consumer stack-to-register move.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not mutate `ideas/closed/338_*` or `ideas/closed/339_*`.
- Do not reopen idea 339 local storage/writeback sizing unless generated-code
  evidence shows the first bad fact has moved there.
- Do not broaden into runtime value correctness, frame layout, aggregate,
  variadic, compare, or semantic `lir_to_bir` work.

## Working Model

Prepared BIR contains both the stack-homed source and the consumer
stack-to-register move needed by the cast. The failure is likely between
prepared move facts and the selected scalar cast operand consumed by the
AArch64 printer: source publication, selected-node construction, normalization,
or move consumption.

## Execution Rules

- Keep routine localization and proof notes in `todo.md`.
- Add focused backend coverage before or with the repair when the behavior can
  be expressed without relying on only the external representative.
- Preserve closed-owner boundaries from ideas 338 and 339 unless fresh evidence
  moves the first bad fact.
- Reclassify any new `00143` residual by generated-code, prepared-state, or
  diagnostic evidence before claiming completion.

## Steps

### Step 1: Localize Scalar Cast Source Publication Boundary

Goal: identify where the prepared stack-to-register consumer move stops being
visible as the selected scalar cast source operand.

Primary target: selected scalar cast lowering/publication for the `00143`
prepared `%t76` to `%t81` shape.

Actions:

- Reproduce the narrow `00143` printer failure and prepared dump evidence.
- Trace the prepared move fact from value id 308 to value id 388 through
  selection, scalar cast record construction, and AArch64 printer handoff.
- Identify whether the missing structured register source is caused by move
  consumption, selected-node construction, operand normalization, or printer
  admission.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete producer, handoff, normalizer, or consumer
  boundary where the structured register source is lost.

### Step 2: Repair Stack-Homed Scalar Cast Register Source Publication

Goal: make selected scalar casts consume the prepared register source when the
original source is stack-homed and a consumer stack-to-register move exists.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the narrow semantic repair without matching `00143`, value ids,
  registers, instruction numbers, or diagnostic strings.
- Add or update focused backend coverage for stack-homed scalar cast source
  publication after consumer stack-to-register moves.
- Preserve existing scalar cast register-source behavior from closed idea 338
  and scalar local storage/writeback behavior from closed idea 339.

Completion check:

- Focused backend coverage proves the selected scalar cast carries a structured
  register source operand into AArch64 printing.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the old printer diagnostic is gone and classify any new first bad
fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm the old scalar cast structured register-source diagnostic is absent.
- If `00143` still fails, classify the new first bad fact from prepared dump,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside this scalar cast source-publication owner.
