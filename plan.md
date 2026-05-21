# Semantic BIR Pointer-Derived String Loads Runbook

Status: Active
Source Idea: ideas/open/356_semantic_bir_pointer_derived_string_loads.md

## Purpose

Repair the semantic-BIR residual where dynamic pointer-derived byte loads from
string or array data collapse into fixed global-byte loads before AArch64
lowering sees them.

## Goal

Preserve the current pointer value as the memory base for pointer-derived byte
loads, then prove `00173` advances without filename-specific or AArch64-only
workarounds.

## Core Rule

Fix the semantic lowering rule that loses dynamic pointer bases. Do not
special-case `00173`, one string literal, one global symbol, one loop shape, or
one emitted instruction sequence.

## Read First

- `ideas/open/356_semantic_bir_pointer_derived_string_loads.md`
- `tests/c/external/c-testsuite/src/00173.c`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/value_materialization.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `tests/backend/bir/`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Targets

- Primary external representative: `c_testsuite_aarch64_backend_src_00173_c`
- Primary semantic evidence: semantic BIR for `00173` around loop loads such
  as `*b` and `*src`
- Focused coverage target: semantic or backend-route coverage proving a
  pointer-derived byte load keeps its dynamic pointer base before AArch64
  codegen
- Stability contracts:
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_memory_operand_contract`,
  and the focused BIR/lir-to-bir tests touched by the repair

## Non-Goals

- Do not reopen AArch64 address-valued memory or call-argument publication
  from idea 355.
- Do not reopen same-module recursive pointer-formal or callee-saved-home
  publication for `00181`.
- Do not handle frontend admission work for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log handling, or CTest registration.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, or unrelated AArch64 call publication work.
- Do not claim progress from emitted AArch64 reshaping while semantic BIR still
  contains fixed global-byte loads for pointer-derived accesses.

## Working Model

`00173` remains a timeout after earlier AArch64 address, pointer, and pointee
publication repairs. Read-only evidence from idea 356 says the first bad fact
appears before AArch64 lowering: loop expressions such as `*b` and `*src` are
represented as fixed `bir.load_global i8 @.str0` / `@.str0+1` accesses. The
generated loop repeatedly tests the first string byte instead of reading
through the incremented pointer value.

The likely owner is semantic BIR construction for loads whose source address
is derived from a pointer value that originally came from global string or
array data. The repair should keep global-backed constants and direct fixed
global-byte loads valid, while dynamic `*p` / `*src` style accesses use the
current pointer as the memory base.

## Execution Rules

- Start by localizing the first semantic-BIR boundary that turns a
  pointer-derived load into a fixed global-byte load.
- Compare source, LIR, semantic BIR, and any prepared/backend-route dumps
  before editing.
- Prefer semantic or backend-route coverage that fails before the repair and
  passes after it.
- Keep direct fixed string literal/global-byte loads working when the source
  expression is actually fixed.
- Treat filename matching, string-literal matching, timeout policy changes,
  runner changes, and expectation weakening as route failure.
- If the repair exposes a later AArch64 lowering failure, classify that new
  first bad fact in `todo.md` before considering a lifecycle split.

## Step 1: Localize The Semantic Dynamic-Load Loss

Goal: identify the exact semantic lowering boundary where a dynamic
pointer-derived byte load becomes a fixed global-byte load.

Actions:

- Reproduce the focused evidence for `00173` and capture semantic BIR around
  the loops containing `*b`, `*src`, or equivalent pointer-derived byte loads.
- Inspect the LIR-to-BIR memory and global lowering path that chooses
  `LoadGlobalInst` versus a load through an address value.
- Record the source expression, pointer carrier, expected dynamic memory base,
  actual BIR load form, and downstream loop consumer.
- Check at least one nearby dynamic pointer-byte shape or existing focused test
  so the owner is not only the named representative.

Completion check:

- `todo.md` records the first bad semantic boundary, expected dynamic base,
  actual fixed global load, producer path, consumer path, and owner
  classification.
- The evidence distinguishes dynamic pointer-derived loads from legitimate
  fixed global-byte loads.

## Step 2: Add Focused Semantic Coverage

Goal: lock down the dynamic pointer-derived load shape before changing shared
lowering behavior.

Actions:

- Add or update the narrowest semantic or backend-route test that can observe
  the BIR load shape before AArch64 codegen.
- Assert that a load through an incremented pointer uses the current pointer
  value as the memory base instead of a fixed global symbol and byte offset.
- Preserve existing fixed global/string literal coverage so direct constant
  loads do not regress.

Completion check:

- The focused test fails on the old fixed-global load behavior and names the
  dynamic pointer-base contract clearly.
- Existing adjacent semantic/global-load tests still pass after the test-only
  change, or any expected pre-repair failure is recorded in `todo.md`.

## Step 3: Repair Pointer-Derived Byte Load Lowering

Goal: make semantic BIR preserve the dynamic pointer base for loads through
current pointer values.

Actions:

- Update only the localized semantic lowering, provenance, or memory-address
  helper needed for the classified owner.
- Keep fixed global-byte loads for truly fixed global/string accesses.
- Avoid AArch64-only patching; AArch64 should benefit because semantic BIR now
  carries the correct dynamic memory operation.
- Re-run the focused coverage from Step 2.

Completion check:

- Focused coverage proves the pointer-derived byte load keeps a dynamic base.
- Direct fixed global/string load behavior remains stable.
- `git diff --check` passes.

## Step 4: Prove Representative Progress And Decide Lifecycle

Goal: prove the semantic repair advances `00173` and decide whether any
remaining first bad fact belongs to this source idea.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`
- Add any focused semantic test names touched in Steps 2 and 3 to the executor
  proof command.
- If `00173` still fails, classify the new first bad fact by semantic BIR,
  prepared BIR, generated AArch64, or runtime evidence.
- Split a new source idea only if the remaining failure is outside dynamic
  pointer-derived load preservation.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00173` advances beyond fixed global-byte loads for dynamic pointer-derived
  accesses or passes.
- Any remaining failure is classified against the source idea's scope before
  closure is requested.
