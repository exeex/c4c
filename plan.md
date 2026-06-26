# RV64 `va_list` Value Publication/Copy Runbook

Status: Active
Source Idea: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Activated from: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md

## Purpose

Resolve the RV64 object-route runtime abort exposed after idea 389 fixed
`va_start` destination-address materialization. The initialized `va_list`
value must be the value observed by later `va_list` copy/call-argument uses.

## Goal

Make later `dummy(statep->ap)` / `dummy(state.ap)`-style `va_list` uses observe
the `va_list` value initialized by `va_start`, or fail closed with a precise
diagnostic owner.

## Core Rule

Use prepared/BIR/object-emission facts to connect the initialized `va_list`
value to later copy or call-argument uses. Do not hard-code
`va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the current abort
branch as the route condition.

## Read First

- `ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md`
- `ideas/closed/389_rv64_va_start_destination_va_list_address_publication.md`
- `build/agent_state/389_step5_va-arg-13.case.log`
- `build/agent_state/389_step5_va-arg-13.objdump.log`
- `build/agent_state/389_step5_va_arg_13.prepared_bir.log`
- `build/agent_state/389_step5_va_arg_13.dummy.prepared_bir.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- RV64 object route for `va_start`-initialized `va_list` value publication
- Later `va_list` copy/call-argument setup for `dummy(statep->ap)` and
  `dummy(state.ap)`
- Focused RV64 backend object-emission tests and CMake wiring as needed

## Non-Goals

- Do not reopen `va_start` destination-address materialization from idea 389.
- Do not reopen `llvm.va_end.p0` lowering from idea 388.
- Do not reopen frame-slot-address GPR call-argument lowering from idea 386.
- Do not implement same-module memory-return/sret calls owned by idea 387.
- Do not perform broad aggregate rewrites, full `va_arg` redesign, or generic
  call ABI changes beyond the `va_list` value publication/copy route.
- Do not downgrade expectations or suppress the `abort()` path without proving
  the initialized `va_list` value reaches the later consumer.

## Working Model

Idea 389 made the prepared destination address valid before
`fragment_for_prepared_variadic_va_start` stores through it. Step 5 evidence
shows the representative now emits:

```text
addi s1,sp,72
addi t1,sp,72
sd t1,0(s1)
```

The next failure is a later runtime abort:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0
c4c_exit=Subprocess aborted
```

The suspected gap is value publication, not address materialization: the
prepared `va_start` destination value is initialized, but later `va_list`
copy/call-argument setup appears to load or copy from the ordinary aggregate
member storage path instead of from the initialized prepared destination value.

## Execution Rules

- Capture prepared/BIR/object facts before choosing a publication or copy
  route.
- Prove the authoritative initialized `va_list` value and the later consumer
  path; do not rely only on the representative abort result.
- Add focused backend coverage before or alongside implementation.
- Preserve fail-closed diagnostics for ambiguous or unsupported `va_list`
  value copy/publication shapes.
- Keep any later boundary separate and record it in `todo.md` for lifecycle
  routing.

## Steps

### Step 1: Capture The `va_list` Value Publication Boundary

Goal: identify the exact prepared/BIR/object facts for the initialized
`va_list` value and the later failing call-argument uses.

Primary target: `va-arg-13.c` prepared/BIR/object evidence around `va_start`,
`dummy(statep->ap)`, `dummy(state.ap)`, and `dummy`.

Actions:

- Inspect the idea 389 Step 5 logs and current prepared/BIR dumps.
- Map the `va_start` initialized value to its prepared destination storage and
  any aliases or publication facts.
- Map the later `dummy` call-argument setup to the value source it currently
  copies.
- Identify whether the ordinary aggregate/member path, prepared destination
  slot, or a missing alias/publication fact is authoritative.
- Record the exact fact gap or ambiguity in `todo.md`.

Completion check: `todo.md` names the initialized `va_list` value, later
consumer value, prepared facts, object slots/registers as evidence, and the
likely owner for publication/copy.

### Step 2: Classify The Publication/Copy Route

Goal: choose the narrow semantic route for connecting the initialized
`va_list` value to later uses.

Primary target: RV64 object emission and prepared value/source metadata.

Actions:

- Decide whether to publish the initialized value back to ordinary
  aggregate/member storage, copy from the prepared destination slot at use
  sites, or model the relationship as a prepared value alias.
- Define required facts and unsupported/ambiguous variants.
- Keep idea 386, idea 387, idea 388, and idea 389 routes out of this owner
  decision.
- Record the route, guards, and fail-closed diagnostics in `todo.md`.

Completion check: `todo.md` states the selected route and exact guard
conditions an executor can implement without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: prove later `va_list` copy/call-argument uses observe the initialized
`va_start` destination value.

Primary target: focused RV64 backend object-emission tests/fixtures and CMake
wiring as needed.

Actions:

- Add or extend focused RV64 object-emission coverage for a supported
  `va_start`-initialized `va_list` value later copied or passed as an argument.
- Assert that the later consumer value comes from the initialized
  destination/publication route.
- Add adjacent fail-closed coverage for ambiguous value-source or alias facts.
- Avoid assertions tied to `va-arg-13.c`, `test`, `dummy`, concrete registers,
  literal offsets, or the abort branch except where fixture-owned expected
  encodings require concrete values.

Completion check: focused coverage pins the current unsafe value-source route
and passes only when the initialized `va_list` value reaches the later
consumer.

### Step 4: Implement Narrow `va_list` Value Publication/Copy

Goal: implement the selected route without broad variadic, aggregate, or ABI
rewrites.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp` and
directly related focused backend wiring if needed.

Actions:

- Use prepared/BIR/object facts to publish or copy the initialized `va_list`
  value to later consumers.
- Emit a narrow diagnostic or unsupported route when required facts are absent
  or ambiguous.
- Preserve idea 386, 387, 388, and 389 behavior.
- Run the delegated backend proof command and record results in `todo.md`.

Completion check: focused backend tests pass, fail-closed variants remain
closed, and the proof log records the exact command and result.

### Step 5: Rerun `va-arg-13.c` And Route The Next Boundary

Goal: verify the representative advances past the current runtime abort or
record a narrower diagnostic owner.

Primary target: `tests/c/external/gcc_torture/src/va-arg-13.c`.

Actions:

- Rerun the representative GCC torture case with the same comparison shape
  used at the idea 389 close boundary.
- Confirm whether the c4c RV64 object route advances past the current
  `Subprocess aborted` mismatch.
- If a later boundary appears, record exact evidence and route it to an
  existing or new idea instead of expanding this plan.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records the representative result, proof logs, and
either completion evidence for idea 390 or a clearly owned later boundary.
