# RV64 External Empty Stub Policy And Runtime Calls Plan

Status: Active
Source Idea: ideas/open/313_rv64_external_empty_stub_policy_runtime.md

## Purpose

Define and implement the RV64 policy for external libc, libm, string, and
user-declared calls so generated programs either link to a real runtime symbol
or fail explicitly before execution.

## Goal

Remove executable bodyless external-call stubs from RV64 output while proving
supported runtime calls and unsupported external calls with focused tests and
representative c-testsuite probes.

## Core Rule

Do not add fake named implementations for observed libc, libm, string, or user
external symbols. Progress must come from a general external-call emission and
diagnostic policy.

## Read First

- ideas/open/313_rv64_external_empty_stub_policy_runtime.md
- build/rv64_c_testsuite_probe_latest/triage_step4/summary.md, if present
- build/rv64_c_testsuite_probe_latest/triage_step4/probe_results.tsv, if
  present
- RV64 module/function/call emission code under
  `src/backend/mir/riscv/codegen/`
- Frontend/backend diagnostic paths used for unsupported target features
- Existing backend RV64 call and codegen-route tests under `tests/backend/`

## Scope

- RV64 emission policy for declarations and definitions of external functions.
- Direct calls to runtime-provided libc, libm, string, and user external
  symbols.
- Prevention of executable empty labels for unresolved external functions.
- Explicit unsupported diagnostics for external calls the compiler cannot
  safely emit or link in the target probe environment.
- Negative tests for unsupported external calls and positive tests for
  supported runtime-link behavior.

## Non-Goals

- Implementing broad libc, libm, or string routines inside the compiler.
- Filename-specific, symbol-specific, or c-testsuite-shaped fake stubs.
- Reclassifying all external calls as unsupported without preserving supported
  runtime-link behavior where the target platform supplies the symbol.
- Ordinary RV64 control/expression, local address, global storage, or scalar
  storage repair except where required to keep external-call policy tests
  isolated.
- Expectation rewrites, qemu skips, or unsupported downgrades claimed as
  runtime progress.

## Working Model

The triage evidence describes `.globl` labels for unresolved external
functions with no body. Calls can then fall into labels such as `strlen:` and
execute accidental empty stubs or adjacent emitted code. Treat this as a
module-symbol policy failure first: decide when a symbol is a definition, when
it is an external declaration/call relocation, and when it must be rejected as
unsupported.

## Execution Rules

- Start with evidence across string, stdio/libc-style, math, and
  user-declared external shapes before changing emission.
- Add tests for policy behavior, not only c-testsuite filenames.
- For supported calls, prove emitted assembly links through the target runtime
  and reaches qemu without accidental local body labels.
- For unsupported calls, prove the compiler stops before producing a runnable
  binary and reports a clear unsupported external-call diagnostic.
- Keep runtime probe results classified by policy decision: supported-linked,
  unsupported-diagnostic, or separate backend residual.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact, and leave canonical regression-log policy to the
  supervisor.

## Step 1: Normalize External Call Policy Evidence

Goal: classify the external-call bucket into supported runtime-link cases,
unsupported external surfaces, and non-policy residuals.

Primary targets:

- `src/00025.c`
- `src/00056.c`
- `src/00125.c`
- `src/00179.c`
- `src/00220.c`

Actions:

- Reprobe the primary targets for emit, clang/link, and qemu outcome.
- Capture BIR/prepared BIR, emitted RV64 assembly, and symbol tables for each
  representative.
- Identify exactly where bodyless labels are introduced and whether each symbol
  should be a runtime-linked external or an unsupported diagnostic.
- Choose the first repair boundary and record any candidates that are not
  external-empty-stub failures.

Completion check:

- `todo.md` names the selected first repair boundary, the supported versus
  unsupported policy split, and representative tests to add.

## Step 2: Add Policy Coverage For External Calls

Goal: encode the intended supported and unsupported behavior before changing
  emission.

Primary target:

- Focused backend/codegen-route cases covering string, stdio/libc-style, math,
  and user-declared external call shapes.

Actions:

- Add positive coverage for at least one runtime-provided external call that
  should link through the target runtime without a local body label.
- Add negative coverage for an unresolved or unsupported external call that
  must fail with an explicit diagnostic before execution.
- Add checks that catch `.globl name` followed by `name:` empty local bodies for
  unresolved external declarations.
- Keep tests semantic and policy-oriented rather than tied to candidate
  filenames.

Completion check:

- Focused tests expose the current empty-stub behavior or diagnostic gap and
  document which assertions are expected to move after repair.

## Step 3: Stop Emitting Executable Empty External Bodies

Goal: separate external declarations/call relocations from local function
definitions in RV64 assembly emission.

Primary target:

- RV64 module/function symbol emission and direct-call lowering.

Actions:

- Inspect the path that emits function labels and `.globl` directives for both
  defined functions and unresolved external declarations.
- Change RV64 emission so declarations used only as external callees do not
  receive executable local body labels.
- Preserve real definitions for functions owned by the compiled translation
  unit.
- Run focused policy tests and a representative runtime probe including
  `src/00025.c`.

Completion check:

- Supported selected calls link/run through the target runtime or fail only
  for a separately classified reason, and emitted assembly no longer contains
  executable empty bodies for unresolved external callees.

## Step 4: Implement Unsupported External Diagnostics

Goal: reject external calls that cannot be safely emitted in the current target
environment with a clear diagnostic.

Primary targets:

- User-declared unresolved externals and runtime surfaces unavailable to the
  probe toolchain.

Actions:

- Define the diagnostic boundary for unsupported external calls without
  weakening supported runtime-link behavior.
- Add or route an explicit unsupported diagnostic before runnable assembly is
  produced for unsupported external surfaces.
- Prove negative tests fail for the diagnostic, not for linker accidents or
  qemu behavior.

Completion check:

- Unsupported representatives fail before execution with a stable diagnostic,
  and supported representatives still use target runtime linkage.

## Step 5: Probe Full External Candidate Bucket

Goal: measure candidate movement and classify remaining failures by policy.

Primary targets:

- All candidate cases listed in the source idea.

Actions:

- Reprobe the full external-call candidate set for emit, clang/link, and qemu
  outcome.
- Summarize each residual as supported-linked failure, unsupported-diagnostic,
  or separate backend/runtime mechanism.
- Add follow-up ideas only for residuals that are outside external empty-stub
  policy.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Source idea acceptance criteria are satisfied: empty executable external
  stubs are gone, policy coverage exists for string/stdio-libc/math/user extern
  shapes, and remaining candidates are classified with concrete evidence.
