# RV64 Object Data Symbol Fixup And Module Assembly Cleanup Runbook

Status: Active
Source Idea: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md

## Purpose

Transcribe the late RV64 object data, symbol/fixup, relocation, and module
assembly cleanup idea into narrow execution steps.

## Goal

Move or narrow late object-emission boundaries only where the interfaces expose
stable fragment, symbol, and fixup contracts without changing emitted object
behavior.

## Core Rule

Preserve object bytes, relocations, symbol bindings, section layout, ELF flags,
public entrypoints, and zero-fill behavior. If a boundary cannot move without
hiding central coupling behind a broad context or callback bundle, record the
no-code decision in `todo.md` and leave the code in place.

## Read First

- `ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/object_emission.hpp`
- possible new compiled object-data or object-module assembly files if a
  narrow owner is justified

## Non-Goals

- Do not repair semantic RV64 lowering capability in this plan.
- Do not change gcc_torture expectations, unsupported markers, runtime
  contracts, or expected outputs.
- Do not combine final module assembly with call, scalar, memory, select, or
  function traversal movement.
- Do not change object bytes, relocations, ELF flags, symbol bindings, section
  names, section alignment, public entrypoints, or zero-fill behavior.
- Do not merge text fixup handling with data-object pointer relocation handling
  before stable producer contracts justify it.

## Working Model

- Earlier fragment producers already expose the safe call, scalar, select, and
  function traversal boundaries that should move in this cleanup series.
- The remaining late object region is high-risk central assembly code. Treat
  movement as optional and proof-driven.
- Data-object emission, text relocation attachment, symbol/fixup publication,
  and final ELF/module assembly should be reviewed as distinct ownership
  boundaries unless one very small helper clearly preserves all contracts.

## Execution Rules

- Keep each step behavior-preserving.
- Prefer narrow helpers with explicit inputs and result types over broad
  context structs.
- Keep public object/ELF entrypoints API-compatible if internals move.
- Use `todo.md` for per-packet progress, proof, blockers, and no-code
  rationale.
- Code-changing steps require:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|object_model_records|cli_riscv64_.*obj|obj_runtime_rv64_|rv64_roundtrip_contract)'
```

- Escalate to broader `^backend_` proof before accepting any slice that changes
  object module layout, relocation attachment, section emission, or ELF writing.

## Steps

### Step 1: Map Late Object Assembly Ownership

Goal: Identify which late object-emission responsibilities are still central
and which, if any, have a narrow extraction boundary.

Actions:
- Inspect data object emission, zero-fill reservation, section selection,
  symbol binding, text fixup attachment, data relocation handling, ELF config,
  and public object entrypoints.
- Record owned helpers and retained central responsibilities in `todo.md`.
- Do not move code in this step unless the boundary is trivial and already
  explicit.

Completion Check:
- `todo.md` names the current late-boundary ownership map, the first executable
  sub-slice, and the exact proof command or no-code rationale.

### Step 2: Review Data Object And Relocation Boundary

Goal: Extract or narrow data-object emission only if data relocations and
zero-fill behavior remain explicit.

Actions:
- Inspect data section emission, pointer relocation creation, symbol names,
  alignments, and zero-fill reservations.
- If safe, extract a small data-emission helper or owner with explicit inputs
  and outputs.
- If unsafe, keep data emission central and record why movement would obscure
  relocation or layout contracts.

Completion Check:
- Behavior-preserving code change is proven by the narrow object test subset,
  or `todo.md` records a no-code decision tied to specific coupling.

### Step 3: Review Text Fixup And Symbol Publication Boundary

Goal: Narrow text fixup and symbol publication ownership only if relocation
attachment and symbol bindings stay observable.

Actions:
- Inspect local label publication, undefined symbol handling, text relocation
  mapping, and symbol binding.
- Separate text fixup handling from data-object pointer relocations unless a
  stable shared contract already exists.
- Preserve public wrappers and emitted object behavior.

Completion Check:
- Any movement has explicit fixup/symbol inputs and narrow proof, or `todo.md`
  records why the central owner remains correct.

### Step 4: Review Final Module Assembly Boundary

Goal: Decide whether final ELF/module assembly can move behind an API-compatible
wrapper without hiding section layout or relocation ownership.

Actions:
- Inspect final object assembly, ELF config, section ordering, section flags,
  relocation attachment, public entrypoints, and result construction.
- Extract only a tiny module assembly owner if its contract is narrower than
  the current central code.
- Escalate to broader `^backend_` proof if layout, section emission, relocation
  attachment, or ELF writing changes.

Completion Check:
- Public object/ELF entrypoints remain compatible, behavior is proven, or
  `todo.md` records a no-code decision for retained central ownership.

### Step 5: Review And Close Readiness

Goal: Decide whether the source idea is satisfied by completed movement and
explicit retained-boundary decisions.

Actions:
- Review `todo.md` for each step's moved code, retained code, proof, and
  no-code rationale.
- Confirm no semantic RV64 fixes, expectation changes, unsupported marker
  changes, or testcase-shaped shortcuts entered the slice.
- Prepare close-readiness notes for plan-owner evaluation.

Completion Check:
- `todo.md` records close readiness, remaining out-of-scope risks if any, and
  the validation state needed for lifecycle close.
