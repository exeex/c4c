# AArch64 Pointer-Derived Address/Lvalue Lowering Authority Runbook

Status: Active
Source Idea: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md

## Purpose

Repair the AArch64 backend path that derives concrete pointer, array, and
subobject addresses for lvalue loads, stores, compound updates, and
address-valued call arguments.

## Goal

Make pointer-derived lvalue consumers use the authoritative concrete address
instead of uninitialized pointer registers, out-of-frame stack offsets, wrong
element offsets, or fallback slots.

## Core Rule

Fix semantic address derivation and publication. Do not special-case named
c-testsuite files or broaden into deferred frontend, timeout,
floating/conversion, string/library-only, aggregate initializer, or closed-owner
overlap buckets.

## Read First

- `ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md`
- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `ideas/closed/293_aarch64_side_effect_control_value_publication_authority.md`
- Current lifecycle state:
  - `todo.md`
- Proof artifacts if still present:
  - `test_before.log`
  - `test_after.log`
  - `/tmp/c4c_aarch64_post293_inventory_scan.log`

## Current Targets

- Starter representatives: `src/00217.c`, `src/00032.c`, `src/00130.c`, and
  `src/00180.c`.
- Address families: local arrays, global data, array-to-pointer decay, pointer
  arithmetic, pointer increments, pointer-to-array indexing, pointer casts, and
  subobject offsets.
- Consumer families: lvalue loads, lvalue stores, compound lvalue updates, and
  address-valued direct-call arguments.
- Later nearby sampling after the first fix: `src/00019.c`, `src/00137.c`,
  `src/00138.c`, and pointer-heavy string or indirect-call cases if generated
  code shows the same address authority failure.

## Non-Goals

- Do not touch implementation files, tests, CTest registration, runner
  behavior, expected outputs, allowlists, unsupported classifications, timeout
  policy, or build/test infrastructure.
- Do not treat frontend/admission failures as this backend owner.
- Do not treat timeout or hang cases such as `src/00220.c` as ordinary runtime
  mismatches.
- Do not absorb floating or scalar-conversion cases such as `src/00174.c`,
  `src/00175.c`, and `src/00119.c`.
- Do not claim string/library-only behavior as fixed unless generated-code
  evidence shows wrong pointer-derived address publication was the cause.
- Do not absorb aggregate/global initializer work such as `src/00050.c`.
- Do not reopen closed-owner overlap such as `src/00159.c`, `src/00168.c`,
  `src/00193.c`, and `src/00196.c` without contradictory generated-code proof.

## Working Model

The AArch64 backend has learned several scalar and call-value authorities, but
some pointer/address consumers still do not receive the concrete memory address
they semantically require. The implementation should repair the shared address
derivation/publication path and then prove it on multiple pointer/array shapes
instead of encoding one testcase shape.

## Execution Rules

- Start from generated AArch64 assembly for the four starter representatives
  and identify the first common authority break before editing code.
- Prefer existing backend/prealloc address facts and local helper patterns over
  new ad hoc testcase matching.
- Keep lvalue address identity intact across load, compute, and store when an
  update expression consumes and writes the same addressed object.
- For direct calls, distinguish address-valued arguments from scalar value
  arguments and publish the object or subobject address to the ABI argument
  register.
- Update `todo.md` with each executor packet's owned files, proof command, and
  generated-assembly observations.
- Use `test_after.log` as the executor proof log unless the supervisor delegates
  another artifact.

## Steps

### Step 1: Locate the Shared Address Authority Break

Goal: identify where pointer/array object identity stops being converted into
the concrete AArch64 address consumed by loads, stores, updates, or calls.

Primary target: generated AArch64 for `src/00217.c`, `src/00032.c`,
`src/00130.c`, and `src/00180.c`.

Actions:

- Inspect source shape and generated assembly for each starter.
- Trace the lowering path for local array decay, global data address plus
  offset, pointer increments, pointer-to-array indexing, pointer casts, and
  address-valued call arguments.
- Identify the smallest shared backend/prealloc surface that chooses fallback
  slots, stale scratch registers, uninitialized pointer registers, or wrong
  element offsets instead of the semantic address.
- Record the exact first repair target and narrow proof command in `todo.md`.

Completion check:

- `todo.md` names the common address/lvalue authority break, the files expected
  to change, the starter subset command, and the generated-assembly symptoms
  that must disappear.

### Step 2: Repair Core Pointer-Derived Lvalue Address Publication

Goal: make lvalue loads and stores consume the concrete pointer-derived address
  for the starter family.

Primary target: the backend/prealloc surface identified in Step 1.

Actions:

- Repair address derivation for local/global objects, array decay, pointer
  arithmetic, casts, and subobject offsets without named-case shortcuts.
- Ensure emitted loads and stores use the derived address and the correct
  element scale or byte offset.
- Keep the change narrow to address/lvalue authority and avoid reopening
  scalar control-value or call-register owners.
- Run the delegated starter subset and capture `test_after.log`.

Completion check:

- Starter tests that exercise ordinary pointer-derived loads/stores no longer
  fail from uninitialized pointers, out-of-frame offsets, wrong element
  offsets, or fallback slots, and generated assembly confirms the fixed
  address source.

### Step 3: Repair Compound Lvalue Updates and Address Arguments

Goal: cover the remaining starter shapes where the same derived address is used
  across load-compute-store or published as a direct-call argument.

Primary target: `src/00217.c` compound lvalue update and `src/00180.c`
address-valued `strcpy`/`printf` arguments, plus any same-surface starter
fallout.

Actions:

- Ensure compound updates load the old value from the derived address, compute
  the semantic result, and store back through that same address.
- Ensure address-valued call arguments publish the local/global/subobject
  address, not a stale scalar or uninitialized callee-saved register.
- Run the delegated starter subset and inspect generated assembly before
  widening.

Completion check:

- `src/00217.c`, `src/00032.c`, `src/00130.c`, and `src/00180.c` pass or have
  any remaining failures classified outside this source idea, with assembly
  evidence for correct address derivation and lvalue consumption.

### Step 4: Nearby Same-Family Sampling and Boundary Separation

Goal: prove the repair is semantic while keeping deferred buckets separate.

Primary target: nearby pointer-heavy cases such as `src/00019.c`,
`src/00137.c`, `src/00138.c`, and string or indirect-call cases only when
generated code shows the same pointer-derived address failure.

Actions:

- Sample nearby pointer/address cases selected by the supervisor.
- Compare any residual failures against frontend, timeout,
  floating/conversion, string/library-only, aggregate initializer, and
  closed-owner overlap buckets before claiming them in scope.
- Preserve generated-assembly observations for both wins and separated
  failures in `todo.md`.
- Ask for reviewer scrutiny if the route starts depending on testcase-shaped
  matching or expectation changes.

Completion check:

- The focused starter subset is green or cleanly separated, nearby sampling
  shows same-family address/lvalue wins, deferred buckets remain explicitly out
  of scope, and no progress depends on test expectation or runner changes.
