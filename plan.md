# AArch64 Globals Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md

## Purpose

Move the AArch64 globals shard from descriptive markdown and broad codegen
owners into compiled `globals.cpp` / `globals.hpp` owners while preserving
current address-materialization behavior.

Goal: reconcile `src/backend/mir/aarch64/codegen/globals.md` into real globals
codegen owners, then delete the markdown shard.

Core Rule: this is source ownership redistribution for globals behavior, not a
semantic expansion or test-expectation rewrite.

## Read First

- `ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/globals.md`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`

## Current Targets

- Add `src/backend/mir/aarch64/codegen/globals.hpp`.
- Add `src/backend/mir/aarch64/codegen/globals.cpp`.
- Move globals-family selected-node construction, lowering, and spelling
  bodies out of broad owners where practical.
- Keep broad owners as routers, shared instruction core, or generic printer
  dispatch only.
- Delete `src/backend/mir/aarch64/codegen/globals.md` once its durable content
  has a compiled owner.

## Non-Goals

- Do not redistribute other AArch64 markdown shards.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely
  for this shard.
- Do not hard-code symbol semantics outside prepared/shared authority.
- Do not add named-case shortcuts, testcase-shaped matching, or final assembly
  text patterning.
- Do not weaken tests, mark supported globals paths unsupported, or rewrite
  expectations to claim progress.
- Do not revive the old `x0` text-emitter convention as codegen authority; use
  current prepared facts, allocation results, and machine instruction records.

## Working Model

- Current live globals behavior is represented by `AddressMaterializationRecord`
  and prepared address materialization facts.
- Direct globals and labels must preserve `ADRP` plus low-12 `ADD`.
- GOT globals must preserve GOT-required policy and the GOT `ADRP` / `LDR`
  form.
- TLS globals must preserve local-exec thread-pointer-relative facts through
  `tpidr_el0` and TLS relocations.
- `dispatch.cpp` should identify and route address-materialization work, not
  own the globals-family implementation body.
- `machine_printer.cpp` should dispatch or delegate globals spelling, not keep
  globals-specific spelling bodies when those can live in the globals shard.

## Execution Rules

- Use small behavior-preserving steps and prove each code-moving slice with a
  fresh build or focused backend test.
- Keep `todo.md` as the executor progress scratchpad; do not rewrite this
  runbook for routine packet completion.
- If a proposed change needs semantic expansion beyond the current structured
  address materialization route, stop and record a separate open idea instead
  of expanding this plan.
- Before closing, ensure `globals.md` is deleted and all durable content has an
  equivalent compiled owner or is explicitly obsolete under current pipeline
  facts.

## Steps

### Step 1: Survey Current Globals Ownership

Goal: identify the exact compiled globals behavior and routing currently spread
across broad owners.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/globals.md`

Actions:
- List the address-materialization record types, helper functions, lowering
  entry points, and printer branches that are globals-family behavior.
- Separate generic machine-instruction infrastructure from globals-specific
  behavior.
- Identify focused backend tests or ctest names that currently cover direct
  global, GOT global, label, string-constant, and TLS materialization paths.
- Update `todo.md` with the chosen first implementation packet and proof
  command.

Completion check:
- `todo.md` names the first code-moving packet, the relevant files, and a
  concrete proof command.
- No implementation files have been changed by this survey-only step unless
  the executor was explicitly delegated to start Step 2.

### Step 2: Create Globals Owner Shell

Goal: introduce compiled globals owner files without changing behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- build-system files that enumerate AArch64 codegen sources

Actions:
- Add the globals header/source pair using local codegen namespace and include
  style.
- Expose only the minimal declarations needed for subsequent relocation.
- Wire the new source into the native build.
- Keep all broad-owner behavior in place for this step unless moving a trivial
  declaration is required to compile.

Completion check:
- The project builds with empty or minimal globals owner files.
- No globals behavior has changed.

### Step 3: Move Address-Materialization Record Construction

Goal: relocate globals-family record selection and validation helpers into
`globals.cpp` / `globals.hpp` while keeping shared instruction core generic.

Primary targets:
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`

Actions:
- Move address-materialization kind selection, prepared-record validation, and
  record construction helpers that belong specifically to global, label,
  string, and TLS address materialization.
- Leave generic instruction variant definitions, opcode naming, and shared
  machine-node plumbing in `instruction.*`.
- Preserve diagnostics and policy mismatch checks.

Completion check:
- Existing address-materialization record tests or focused backend tests still
  pass.
- `instruction.cpp` no longer owns globals-family construction bodies except
  where generic instruction infrastructure requires it.

### Step 4: Move Dispatch Lowering Body Behind Globals Owner

Goal: keep `dispatch.cpp` as a router and move the address-materialization
lowering body into the globals owner.

Primary targets:
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:
- Move `lower_address_materialization`-style behavior into the globals owner.
- Keep dispatch ordering and visited-operation behavior unchanged.
- Preserve diagnostics for missing prepared address materialization.
- Keep scalar-register tracking behavior correct at the dispatch boundary.

Completion check:
- Focused backend proof shows lowered direct, GOT, label, string, and TLS
  address-materialization nodes are still selected or rejected exactly as
  before.
- `dispatch.cpp` routes to globals lowering without retaining the globals
  lowering body.

### Step 5: Move Globals Printer Spelling

Goal: move globals-specific address-materialization spelling into the globals
owner while preserving printer dispatch behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- Move direct, GOT, label, string-constant, and TLS address-materialization
  spelling helpers into globals-owned printer helpers.
- Preserve exact emitted forms for `:got:`, `:got_lo12:`, `:lo12:`,
  `:tprel_hi12:`, and `:tprel_lo12_nc:`.
- Keep generic `MachineInstructionPrinter` dispatch and error reporting in the
  printer owner unless the local interfaces require a small delegation helper.

Completion check:
- Focused assembly-output proof shows the same global, label, string, GOT, and
  TLS spelling forms as before.
- `machine_printer.cpp` no longer contains globals-specific spelling bodies
  that can be owned by the globals shard.

### Step 6: Delete Markdown Shard And Run Acceptance Proof

Goal: finish the redistribution and prove behavior was preserved.

Primary targets:
- `src/backend/mir/aarch64/codegen/globals.md`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- focused backend tests

Actions:
- Confirm every durable behavior note in `globals.md` is represented in
  compiled globals owner code or is explicitly obsolete under current structured
  pipeline facts.
- Delete `globals.md`.
- Run focused backend proof for address materialization.
- Escalate to broader backend or full ctest proof if globals changes touch
  shared instruction/printer infrastructure broadly.

Completion check:
- `globals.md` is gone.
- `globals.cpp` and `globals.hpp` own valid globals shard behavior.
- Focused backend proof is green, and broader proof is green if required by
  blast radius.
- The diff contains no unrelated feature expansion or expectation downgrade.
