# RV64 Va List Call-Argument Publication Continuation Runbook

Status: Active
Source Idea: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Supersedes: prior five-step runbook exhausted after Step 5; idea 392 remains open because `va-arg-13.c` still reaches the same value-publication boundary.

## Purpose

Continue idea 392 from the post-Step-5 evidence. Step 4 added explicit prepared
`call_argument_value_publications` and focused backend proof passed, but the
representative still stores the `va_list` storage address into the call
argument object instead of the initialized save-area pointer payload.

## Goal

Make the representative use the prepared `va_list` call-argument publication
fact/effective payload, or identify the exact missing/mismatched fact boundary
that prevents the prepared route from owning the representative.

## Core Rule

Follow the explicit prepared call-argument value-publication facts through the
real representative path. Do not replace this with `va-arg-13.c`, `test`,
`dummy`, register, stack-offset, or abort-branch matching.

## Read First

- `ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md`
- `todo.md`
- `test_after.log`
- `build/agent_state/392_step5_va-arg-13.case.log`
- `build/agent_state/392_step5_va-arg-13.c4c-disasm.log`
- `build/agent_state/392_step5_va-arg-13.clang-disasm.log`
- `build/agent_state/392_step5_va-arg-13.qemu-strace.log`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- Prepared `call_argument_value_publications` emitted for the representative
- Store-source/publication records referenced by those prepared facts
- Backend selection and application of the prepared publication fact at the
  first and second `dummy` call argument objects

## Non-Goals

- Do not reopen RV64 variadic prologue save-area publication from idea 391.
- Do not reopen `va_start` destination-address materialization from idea 389.
- Do not reopen prepared frame-slot-address GPR call-argument lowering from
  ideas 386 or 390 except where the explicit value-publication fact proves a
  direct conflict.
- Do not implement same-module memory-return/sret calls owned by idea 387.
- Do not redesign generic variadic, aggregate, call ABI, or `va_arg` lowering.
- Do not downgrade expectations, mark the representative unsupported, or
  suppress the abort path as proof.

## Working Model

The previous runbook established the right semantic owner and added explicit
prepared `call_argument_value_publications`. The remaining failure is narrower:
the focused fixture exercises the new backend route, but the real
representative still emits:

```text
mv t1,s1
sd t1,24(sp)
...
mv t1,s1
sd t1,32(sp)
```

where `s1` is the local `va_list` storage address (`sp+0x80`). The expected
payload is the initialized save-area pointer stored in that object. The next
route must determine whether the representative lacks the prepared fact, carries
a mismatched source/destination identity, loses the effective payload before
object emission, or falls through to an older address-publication path.

## Execution Rules

- Start from emitted prepared facts for the real representative before editing
  backend behavior.
- Compare the representative against the focused passing fixture and explain
  the divergence.
- Keep the argument object address, local `va_list` storage address, and
  initialized save-area pointer payload distinct in notes and assertions.
- Preserve fail-closed behavior for missing, duplicate, ambiguous, or
  mismatched publication facts.
- Record any later boundary in `todo.md` and route it separately instead of
  expanding this plan.

## Steps

### Step 1: Trace Representative Prepared Publications

Goal: prove whether `va-arg-13.c` carries the explicit prepared
`call_argument_value_publications` needed by the Step 4 backend route.

Primary target: prepared dumps/prints for the first and second `dummy` calls.

Actions:

- Generate or inspect the representative prepared output that includes
  `call_argument_value_publications` and referenced store-source/publication
  records.
- Name the source local object, initialized payload source, destination
  argument object, callsite, and referenced publication fact IDs for both
  `dummy` calls.
- Compare those facts with the focused backend fixture that passed in Step 4.
- Classify the boundary as absent fact, mismatched fact identity, wrong
  effective payload, or backend fallthrough despite a valid fact.
- Record the exact classification and evidence in `todo.md`.

Completion check: `todo.md` states whether the representative has a valid
explicit prepared publication fact for each `dummy` call and why the current
backend does or does not consume it.

### Step 2: Pin The Backend Selection Failure

Goal: identify the exact branch that emits the `va_list` storage address for
the representative after the prepared facts are available.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`.

Actions:

- Trace the call-argument object emission path for the representative from
  prepared fact lookup through final store emission.
- Determine whether the explicit value-publication route is skipped, rejected,
  shadowed by an older frame-slot/address route, or selected with the wrong
  payload.
- Add focused assertions or diagnostics only as needed to make this branch
  observable in backend proof.
- Keep generic `StoreLocalPublication` records as input evidence, not as
  standalone callsite authority.
- Record the selected owner and minimal repair target in `todo.md`.

Completion check: `todo.md` names the failing backend branch/guard and the
minimal semantic repair needed to make the explicit prepared publication route
own the representative.

### Step 3: Repair The Representative Publication Path

Goal: make supported representative-shaped facts copy the initialized
`va_list` payload into the call-argument object without testcase-shaped
matching.

Primary target: direct prepared-publication plumbing and RV64 object emission.

Actions:

- Implement the minimal repair identified in Step 2.
- If the representative lacks required prepared facts, repair fact production
  instead of weakening backend guards.
- If backend matching is wrong, bind by the explicit prepared publication fact
  and its referenced source/destination identities.
- If the payload is wrong, load/copy the initialized source payload rather than
  storing the source object's address.
- Extend focused backend coverage for the representative divergence and
  preserve existing fail-closed variants.

Completion check: focused backend tests cover the old fallthrough/mismatch and
pass only when the initialized `va_list` payload reaches the call argument
object through the explicit prepared route.

### Step 4: Reprove Focused Coverage And Representative

Goal: prove idea 392 acceptance or route a genuinely later boundary.

Primary target: focused backend tests plus `va-arg-13.c`.

Actions:

- Run the delegated backend proof command selected by the supervisor.
- Rerun the `va-arg-13.c` RV64 object-route representative with the same
  comparison shape used in Step 5.
- Confirm whether the prior `Subprocess aborted` boundary is gone.
- If a later boundary appears, record exact logs, owner, and split/defer
  recommendation in `todo.md`.
- Keep `test_after.log` as the canonical executor proof log unless the
  supervisor delegates another artifact.

Completion check: `todo.md` records the focused proof and representative
result, and either shows idea 392 acceptance evidence or identifies a later
owner without weakening the current idea.
