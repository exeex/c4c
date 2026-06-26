# RV64 Va List Expression Call-Argument Value Publication Runbook

Status: Active
Source Idea: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Activated from: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md

## Purpose

Resolve the RV64 object-route runtime abort that remains after incoming
variadic GPR payloads are saved into the `va_start` backing area. A `va_list`
expression passed as a call argument must publish the initialized pointer
payload stored in the `va_list` object, not the address of that storage object.

## Goal

Make supported RV64 `va_list` expression call arguments copy the initialized
save-area pointer payload into the destination argument object, or fail closed
with a precise diagnostic owner.

## Core Rule

Use prepared/BIR/object facts for `va_list` value ownership and call-argument
publication. Do not hard-code `va-arg-13.c`, `test`, `dummy`, stack offsets,
or the current abort branch as the route condition.

## Read First

- `ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md`
- `ideas/open/391_rv64_variadic_prologue_save_area_publication.md`
- `ideas/closed/390_rv64_va_list_value_publication_copy_runtime_abort.md`
- `build/agent_state/391_step5_va-arg-13.case.log`
- `build/agent_state/391_step5_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step5_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step5_va-arg-13.qemu-strace.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- `va_list` expression value publication into call-argument storage
- Prepared/BIR/object facts connecting the initialized local `va_list` payload
  to the destination argument object
- Focused RV64 backend object-emission tests and CMake wiring as needed

## Non-Goals

- Do not reopen RV64 variadic prologue save-area publication from idea 391.
- Do not reopen `va_start` destination-address materialization from idea 389.
- Do not reopen prepared-call frame-slot-address publication from idea 390
  except where this expression/value path needs a distinct prepared fact or
  guard.
- Do not reopen `llvm.va_end.p0` lowering from idea 388.
- Do not reopen ordinary frame-slot-address GPR call-argument lowering from
  idea 386.
- Do not implement same-module memory-return/sret calls owned by idea 387.
- Do not perform broad aggregate rewrites, full `va_arg` redesign, or generic
  call ABI changes beyond the `va_list` expression call-argument value route.
- Do not downgrade expectations or suppress the `abort()` path without proving
  the initialized `va_list` payload reaches the argument object consumed by
  `va_arg`.

## Working Model

Idea 391 made the RV64 variadic prologue store incoming variadic GPR payloads
into the prepared backing area before `va_start` exposes it. Step 5 still
reports:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The remaining evidence points after `va_start`: c4c initializes the local
`va_list` slot with the backing save-area pointer, but the first `dummy`
argument object is built from the `va_list` slot address rather than from that
initialized payload. `dummy` therefore reads through the wrong object and
reaches `abort()`.

## Execution Rules

- Capture prepared/BIR/object facts before choosing a value-publication route.
- Prove the initialized `va_list` payload source and the destination argument;
  do not rely only on the representative abort result.
- Add focused backend coverage before or alongside implementation.
- Preserve fail-closed diagnostics for ambiguous, missing, duplicate, or
  mismatched value-publication facts.
- Keep any later boundary separate and record it in `todo.md` for lifecycle
  routing.

## Steps

### Step 1: Capture The Va List Value Boundary

Goal: identify the exact prepared/BIR/object facts for the initialized
`va_list` payload and the call argument object that should receive it.

Primary target: `va-arg-13.c` prepared/BIR/object and disassembly evidence
around `va_start`, the first `dummy` call, and `dummy`'s `va_arg` load.

Actions:

- Inspect the idea 391 Step 5 logs and current prepared/BIR dumps.
- Map the local `va_list` object, the initialized save-area pointer payload,
  and the destination call argument object for the first `dummy` call.
- Identify whether the gap is missing value publication, an incorrect source
  object/value distinction, or an ambiguous ownership fact.
- Record the exact fact gap or ambiguity in `todo.md`.

Completion check: `todo.md` names the initialized payload source, call
argument destination, prepared facts, object slots/registers as evidence, and
the likely owner for value publication.

### Step 2: Classify The Value-Publication Route

Goal: choose the narrow semantic route for publishing initialized `va_list`
payload values into call-argument objects.

Primary target: RV64 object emission and prepared call/value metadata.

Actions:

- Decide whether publication belongs in an existing prepared-call fragment, a
  distinct prepared `va_list` value-publication fact, or a guarded extension of
  an existing publication route.
- Define required facts and unsupported/ambiguous variants.
- Keep ideas 386, 387, 388, 389, 390, and 391 routes out of this owner
  decision unless the facts prove direct ownership.
- Record the route, guards, and fail-closed diagnostics in `todo.md`.

Completion check: `todo.md` states the selected route and exact guard
conditions an executor can implement without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: prove supported RV64 `va_list` expression call arguments receive the
initialized pointer payload rather than the source object's address.

Primary target: focused RV64 backend object-emission tests/fixtures and CMake
wiring as needed.

Actions:

- Add or extend focused RV64 backend coverage for a `va_list` object passed as
  a call argument after initialization.
- Assert that the initialized payload is copied into the argument object
  through the selected publication route.
- Add adjacent fail-closed coverage for missing, duplicate, ambiguous, or
  malformed publication facts.
- Avoid assertions tied to `va-arg-13.c`, `test`, `dummy`, concrete registers,
  literal offsets, or the abort branch except where fixture-owned expected
  encodings require concrete values.

Completion check: focused coverage pins the current missing value-publication
route and passes only when the initialized `va_list` payload reaches the
expected call argument object.

### Step 4: Implement Narrow Va List Value Publication

Goal: implement the selected route without broad variadic, aggregate, or ABI
rewrites.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp` and
directly related focused backend wiring if needed.

Actions:

- Use prepared/BIR/object facts to copy the initialized `va_list` payload into
  the destination argument object.
- Emit a narrow diagnostic or unsupported route when required facts are absent
  or ambiguous.
- Preserve idea 386, 387, 388, 389, 390, and 391 behavior.
- Run the delegated backend proof command and record results in `todo.md`.

Completion check: focused backend tests pass, fail-closed variants remain
closed, and the proof log records the exact command and result.

### Step 5: Rerun `va-arg-13.c` And Route The Next Boundary

Goal: verify the representative advances past the current runtime abort or
record a narrower diagnostic owner.

Primary target: `tests/c/external/gcc_torture/src/va-arg-13.c`.

Actions:

- Rerun the representative GCC torture case with the same comparison shape
  used at the idea 391 Step 5 boundary.
- Confirm whether the c4c RV64 object route advances past the current
  `Subprocess aborted` mismatch.
- If a later boundary appears, record exact evidence and route it to an
  existing or new idea instead of expanding this plan.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records the representative result, proof logs, and
either completion evidence for idea 392 or a clearly owned later boundary.
