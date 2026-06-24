# Native Object Emission Umbrella Final Review Runbook

Status: Active
Source Idea: ideas/open/329_native_object_emission_umbrella.md
Activated after: ideas/closed/335_textual_assembler_object_route_followup.md

## Purpose

Run the final umbrella acceptance review now that the child queue for native
object emission has closed.

## Goal

Confirm that the closed child ideas compose into the original native object
emission architecture goal, then either close the umbrella or record the exact
remaining gap.

## Core Rule

This plan is review/lifecycle only. Do not implement object-emitter features,
change defaults, weaken tests, or add scan coverage from this runbook.

## Read First

- `ideas/open/329_native_object_emission_umbrella.md`
- `ideas/closed/330_native_object_model_and_emission_api.md`
- `ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md`
- `ideas/closed/332_aarch64_minimal_relocatable_elf_object_emission.md`
- `ideas/closed/333_codegen_obj_cli_and_test_integration.md`
- `ideas/closed/334_object_route_scan_and_default_readiness.md`
- `ideas/closed/335_textual_assembler_object_route_followup.md`
- Follow-up target-emitter children closed during object scan expansion:
  `ideas/closed/336_target_object_emitter_scalar_scan_expansion.md`,
  `ideas/closed/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md`,
  and `ideas/closed/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md`.
- `test_before.log`
- `test_after.log`

## Current Evidence

- Direct `--codegen obj` exists as an explicit CLI route and writes bytes
  through the backend object facade.
- RV64 and AArch64 both have accepted target-owned direct ELF object emission
  for the agreed smoke and scan subsets.
- Existing `.s` route tests remain present and selected.
- Object-route tests cover object structure, diagnostics, linkability, and
  RV64 runtime behavior.
- Object-route scan/default-readiness closed with a 41/41 baseline and a
  reviewed decision to keep object output explicit rather than default for now.
- Textual assembler follow-up closed with a no-work-needed decision because
  direct object output does not depend on textual round-tripping.

## Non-Goals

- Do not make native object output the default in this plan.
- Do not add AArch64 runtime, x86 object output, object stdout, semantic-BIR
  object mode, or c-testsuite default changes.
- Do not broaden target object-emitter support.
- Do not reinterpret known unsupported boundaries as failures of the umbrella
  unless they contradict the accepted child scopes.

## Step 1: Final Acceptance Review

Goal: decide whether the closed child queue satisfies the umbrella completion
criteria.

Actions:

- Compare the umbrella completion criteria against the closed child evidence.
- Confirm all child ideas created from the umbrella are closed.
- Confirm direct `.o` output does not route through printed `.s`.
- Confirm asm-route coverage remains present and meaningful.
- Confirm object-route failures and unsupported combinations remain diagnosable
  through object writer, encoder, relocation, or policy diagnostics.
- Record the final review decision in `todo.md`.

Completion check:

- `todo.md` states whether the umbrella is ready for closure or names the exact
  source-idea gap that prevents closure.

## Step 2: Close Or Repair Umbrella Lifecycle

Goal: finish the umbrella lifecycle based on Step 1.

Actions if closure is accepted:

- Run or reuse the close-scope regression guard.
- Add a compact closure note to
  `ideas/open/329_native_object_emission_umbrella.md`.
- Move the source idea to `ideas/closed/`.
- Remove `plan.md` and `todo.md`.

Actions if closure is rejected:

- Keep the umbrella open.
- Update `todo.md` with the next focused handoff, or create/switch to a child
  only if the remaining work is distinct source intent.

Completion check:

- Either the umbrella is closed with regression evidence, or active lifecycle
  state names the next exact bounded packet.
