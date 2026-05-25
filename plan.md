# AArch64 Call Printing And Effect Publication Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue AArch64 call-emission consolidation after the Step 5 closure review
for the call-argument preservation lookup checkpoint rejected source-idea
closure.

## Goal

Decide and execute the next narrow checkpoint for the surviving
`calls_printing.cpp` call-boundary printing and effect-publication surface,
leaving printer-only work in the machine-printer layer and target-local calls
code responsible only for AArch64 call emission facts.

## Core Rule

Target-local AArch64 calls code may build AArch64 call and call-boundary
machine nodes from prepared call facts. It must not own printer routing,
print-only effect spelling, or duplicate call-boundary classification that
belongs in the machine-printer layer or shared prepared facts.

## Latest Closure Review Finding

The Step 5 closure review after the call-argument preservation lookup
checkpoint rejects source-idea closure.

The checkpoint's broader backend proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
reported 162/162 backend tests passed in the rolled-forward canonical
`test_before.log`.

The source idea remains open because `calls_printing.cpp` is still a large
separate calls-family translation unit with printer-result declarations
exported through `calls.hpp`. It owns call-boundary selection diagnostics,
machine-node effect publication helpers, prepared preserved-value effect
conversion, immediate-cast publication assembly, and call/call-boundary
printing. That surviving printer/effect-publication boundary is durable
remaining work under the source idea's acceptance criteria, especially the
scope item to move call printing or effect spelling that is not AArch64
emission into the appropriate machine-printer or shared MIR layer.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- Focused backend call-boundary, printer, and prepared-call tests under
  `tests/backend/mir/`

## Current Targets / Scope

- `calls_printing.cpp`
- `print_call`
- `print_call_boundary_move`
- `print_call_boundary_abi_binding`
- `make_call_boundary_move_instruction`
- `make_call_boundary_abi_binding_instruction`
- `effect_from_prepared_call_preserved_value`
- `effects_from_prepared_call_preserved_values`
- `call_boundary_move_selection_status`
- `call_boundary_abi_binding_selection_status`
- `call_selection_status`
- Printer declarations in `calls.hpp`
- Adjacent dispatch in `machine_printer.cpp` only where it already routes call
  instruction payloads to call printers

## Non-Goals

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not perform broad dispatch cleanup outside AArch64 call printing and
  call-boundary publication.
- Do not invent a new shared call-plan API.
- Do not move AArch64-specific register, memory operand, frame-slot address, or
  assembly spelling details into shared planning.
- Do not weaken unsupported or expected-output contracts.
- Do not change behavior solely to reduce line count.
- Do not absorb byval, aggregate, local-frame, variadic, ALU, memory,
  comparison, prologue, or unrelated dispatch lowering cleanup into this
  checkpoint.
- Do not revisit completed aggregate-address, local-frame publication,
  prior stack-preservation lookup, block-entry republication, frame-slot call
  argument narrowing, value-id prior-preservation lookup, or call-argument
  prior-preservation lookup routes unless this checkpoint proves one still owns
  duplicate printer/effect authority.

## Working Model

- Treat `calls_printing.cpp` as a mixed boundary: some helpers create
  call-boundary machine-node records, some helpers publish effects, and some
  helpers are pure printing.
- Keep AArch64-specific machine-node construction and operand assembly where
  the emission layer needs them.
- Move or narrow pure printer routing and line spelling toward
  `machine_printer.cpp` when that reduces the calls-family API surface without
  hiding emission behavior.
- If effect publication is needed by both call emission and printing, decide
  whether it belongs on the machine-node record, in the printer layer, or in a
  smaller calls-owned emission helper.
- A surviving helper is acceptable only when its parameters and callers
  describe AArch64 emission-node construction rather than printer ownership or
  prepared-call decision reconstruction.

## Execution Rules

- Start by mapping every exported `calls_printing.cpp` declaration in
  `calls.hpp` to either emission-node construction, machine-printer routing, or
  shared prepared fact consumption.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing printer routing, call-boundary effect
  records, preserved-value effect conversion, inline assembly publication, or
  call-boundary machine instruction printing.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Map The Printing And Effect-Publication Boundary

Goal: identify which surviving `calls_printing.cpp` responsibilities are pure
printer work, which are emission-node construction, and which are duplicate
prepared-call/effect decisions.

Primary targets:

- `calls_printing.cpp`
- printer declarations in `calls.hpp`
- call payload routing in `machine_printer.cpp`
- `make_call_boundary_move_instruction`
- `make_call_boundary_abi_binding_instruction`
- `print_call`
- `print_call_boundary_move`
- `print_call_boundary_abi_binding`
- `effect_from_prepared_call_preserved_value`

Actions:

- Classify each exported helper as emission-node construction, pure printing,
  effect publication, or duplicate prepared-call/effect decision logic.
- Determine whether pure printing helpers can move to `machine_printer.cpp`
  without dragging AArch64 call emission dependencies into generic prepared
  planning.
- Determine whether preserved-value effect conversion should remain calls-owned
  or be represented on machine-node records before printing.
- Identify a focused proof command that covers call printing, call-boundary
  moves, prepared-call preservation effects, and AArch64 call output.
- Record the selected ownership decision, any precise blocker, and the focused
  proof command in `todo.md`.

Completion check:

- `todo.md` names the printer/effect boundary, the selected owner for each
  helper family, any missing machine-node/effect fact blocker, and the focused
  proof scope.

## Step 2: Move Or Narrow Pure Printer Ownership

Goal: remove printer-only declarations from the calls-family API or narrow them
to machine-printer-owned implementation details.

Actions:

- Move pure `print_*` helper bodies to the machine-printer layer when Step 1
  proves they do not need calls-owned emission authority.
- Keep AArch64-specific mnemonic, register, frame-slot, immediate, and inline
  assembly spelling in AArch64 machine-printer code.
- Avoid moving prepared-call planning or preservation selection into the
  printer.
- Preserve diagnostics unless Step 1 proves a stronger machine-node fact makes
  one obsolete.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- Pure call printing no longer requires exported calls-family declarations
  except for helpers that Step 1 classifies as emission-owned, and
  `test_after.log` records a passing build plus focused proof.

## Step 3: Consolidate Effect Publication And Declarations

Goal: shrink the remaining calls printing/effect API to emission-only helpers
or retire it if ownership moved cleanly.

Actions:

- Remove obsolete printer declarations from `calls.hpp`.
- Keep or move preserved-value effect conversion according to the Step 1
  ownership decision.
- Delete includes and translation-unit boundaries made obsolete by Step 2.
- Update build metadata only if a file is retired or moved.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The call printing/effect-publication surface is smaller or explicitly owned
  by the machine-printer layer, with build metadata and include graphs matching
  the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the call printing/effect-publication checkpoint did not regress
adjacent call emission, prepared-call, preservation, byval, aggregate,
local-frame, dispatch-bridge, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, prepared-call, preservation,
  printer-output, argument-source, byval, aggregate, local-frame/publication,
  and dispatch-bridge tests.
- Include affected shared-boundary and x86 tests if shared MIR printer or
  machine-node effect behavior was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied to
  the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this checkpoint or
whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp` and `calls.hpp` boundaries against the
  source idea acceptance criteria.
- Confirm target-local calls code no longer rederives decisions already present
  in shared prepared facts.
- Confirm surviving helper files are emission-only, printer-owned,
  dispatch-owned outside this source idea, or identified as the next checkpoint
  target.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
