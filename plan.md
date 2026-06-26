# RV64 Variadic Prologue Save-Area Publication Runbook

Status: Active
Source Idea: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Activated from: ideas/open/391_rv64_variadic_prologue_save_area_publication.md

## Purpose

Resolve the RV64 object-route runtime abort that remains after the prepared
`va_list` destination-address and prepared-call publication/copy routes are
fixed. The incoming variadic argument payload must be available in the backing
save area consumed by `va_start` / `va_arg`.

## Goal

Make supported RV64 variadic callees publish incoming variadic GPR payloads
into the save area reached by the initialized `va_list`, or fail closed with a
precise diagnostic owner.

## Core Rule

Use prepared/BIR/object facts for variadic prologue payload ownership and
save-area publication. Do not hard-code `va-arg-13.c`, `test`, `dummy`,
register names, stack offsets, or the current abort branch as the route
condition.

## Read First

- `ideas/open/391_rv64_variadic_prologue_save_area_publication.md`
- `ideas/closed/390_rv64_va_list_value_publication_copy_runtime_abort.md`
- `build/agent_state/390_step5_va-arg-13.case.log`
- `build/agent_state/390_step5_va-arg-13.c4c_disasm.log`
- `build/agent_state/390_step5_va-arg-13.clang_disasm.log`
- `build/agent_state/390_step5_va-arg-13.qemu_strace.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- RV64 variadic callee prologue / save-area setup
- Incoming variadic GPR payload publication for later `va_start` / `va_arg`
  consumption
- Focused RV64 backend object-emission tests and CMake wiring as needed

## Non-Goals

- Do not reopen `va_start` destination-address materialization from idea 389.
- Do not reopen prepared-call frame-slot-address `va_list` value
  publication/copy from idea 390.
- Do not reopen `llvm.va_end.p0` lowering from idea 388.
- Do not reopen ordinary frame-slot-address GPR call-argument lowering from
  idea 386.
- Do not implement same-module memory-return/sret calls owned by idea 387.
- Do not perform broad aggregate rewrites, full `va_arg` redesign, or generic
  call ABI changes beyond the RV64 variadic save-area publication route.
- Do not downgrade expectations or suppress the `abort()` path without proving
  incoming variadic payloads reach the backing save area consumed by `va_arg`.

## Working Model

Idea 390 made later prepared-call frame-slot-address arguments copy the
initialized `va_list` pointer payload into the call argument object. Step 5
still reports:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The current evidence points one boundary earlier than `dummy`'s read: `dummy`
receives a va-list pointer object, but the pointed-to save area does not
appear to contain the incoming variadic `a1=1234` payload that clang publishes
for later `va_arg` consumption.

## Execution Rules

- Capture prepared/BIR/object facts before choosing a save-area publication
  route.
- Prove the incoming variadic payload source and the later save-area consumer;
  do not rely only on the representative abort result.
- Add focused backend coverage before or alongside implementation.
- Preserve fail-closed diagnostics for ambiguous or unsupported variadic
  prologue save-area shapes.
- Keep any later boundary separate and record it in `todo.md` for lifecycle
  routing.

## Steps

### Step 1: Capture The Variadic Save-Area Boundary

Goal: identify the exact prepared/BIR/object facts for incoming RV64 variadic
payloads and the save area later reached by `va_start` / `va_arg`.

Primary target: `va-arg-13.c` prepared/BIR/object and disassembly evidence
around `test` prologue, `va_start`, and `dummy`.

Actions:

- Inspect the idea 390 Step 5 logs and current prepared/BIR dumps.
- Map incoming variadic argument payloads, especially the payload corresponding
  to `a1=1234`, to any prepared prologue or save-area facts.
- Map the `va_start` initialized pointer to the save area later consumed by
  `dummy` / `va_arg`.
- Identify whether the gap is missing prologue save-area publication, an
  incorrect save-area base, or an ambiguous ownership fact.
- Record the exact fact gap or ambiguity in `todo.md`.

Completion check: `todo.md` names the incoming payload source, save-area
destination, prepared facts, object slots/registers as evidence, and the
likely owner for save-area publication.

### Step 2: Classify The Save-Area Publication Route

Goal: choose the narrow semantic route for publishing incoming variadic
payloads into the save area.

Primary target: RV64 object emission and prepared variadic prologue metadata.

Actions:

- Decide whether publication belongs in the variadic prologue object fragment,
  a prepared local/save-area publication route, or a new prepared fact consumed
  by existing object emission.
- Define required facts and unsupported/ambiguous variants.
- Keep ideas 386, 387, 388, 389, and 390 routes out of this owner decision.
- Record the route, guards, and fail-closed diagnostics in `todo.md`.

Completion check: `todo.md` states the selected route and exact guard
conditions an executor can implement without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: prove supported RV64 incoming variadic payloads reach the backing save
area consumed by `va_arg`.

Primary target: focused RV64 backend object-emission tests/fixtures and CMake
wiring as needed.

Actions:

- Add or extend focused RV64 backend coverage for a variadic callee with an
  incoming GPR variadic payload consumed through `va_start` / `va_arg`.
- Assert that the payload is stored into the save area through the selected
  publication route.
- Add adjacent fail-closed coverage for missing, ambiguous, or malformed
  save-area publication facts.
- Avoid assertions tied to `va-arg-13.c`, `test`, `dummy`, concrete registers,
  literal offsets, or the abort branch except where fixture-owned expected
  encodings require concrete values.

Completion check: focused coverage pins the current missing save-area
publication route and passes only when incoming variadic payloads reach the
expected save area.

### Step 4: Implement Narrow Save-Area Publication

Goal: implement the selected route without broad variadic, aggregate, or ABI
rewrites.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp` and
directly related focused backend wiring if needed.

Actions:

- Use prepared/BIR/object facts to publish incoming variadic payloads into the
  backing save area.
- Emit a narrow diagnostic or unsupported route when required facts are absent
  or ambiguous.
- Preserve idea 386, 387, 388, 389, and 390 behavior.
- Run the delegated backend proof command and record results in `todo.md`.

Completion check: focused backend tests pass, fail-closed variants remain
closed, and the proof log records the exact command and result.

### Step 5: Rerun `va-arg-13.c` And Route The Next Boundary

Goal: verify the representative advances past the current runtime abort or
record a narrower diagnostic owner.

Primary target: `tests/c/external/gcc_torture/src/va-arg-13.c`.

Actions:

- Rerun the representative GCC torture case with the same comparison shape
  used at the idea 390 close boundary.
- Confirm whether the c4c RV64 object route advances past the current
  `Subprocess aborted` mismatch.
- If a later boundary appears, record exact evidence and route it to an
  existing or new idea instead of expanding this plan.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records the representative result, proof logs, and
either completion evidence for idea 391 or a clearly owned later boundary.
