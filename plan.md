# AArch64 String Literal Pointer Value Publication Runbook

Status: Active
Source Idea: ideas/open/365_aarch64_string_literal_pointer_value_publication.md

## Purpose

Repair the downstream AArch64/prepared-addressing residual where a string
literal assigned to a local pointer publishes a stack-spill address instead of
the literal/global address.

## Goal

Materialize string literal or global data addresses as pointer values before
local storage and dynamic pointer consumers use them, then prove `00173`
advances beyond the current segmentation fault without weakening test
contracts.

## Core Rule

Fix general string/global address pointer-value publication. Do not
special-case `00173`, `.str0`, one source string, one stack offset, one local
slot, one ABI register, one loop body, or one emitted instruction sequence.

## Read First

- `ideas/open/365_aarch64_string_literal_pointer_value_publication.md`
- `ideas/open/356_semantic_bir_pointer_derived_string_loads.md`
- `tests/c/external/c-testsuite/src/00173.c`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/value_materialization.cpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- `src/backend/prealloc/regalloc/spill_reload.cpp`
- `src/backend/prealloc/prepared_printer/addressing.cpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/bir/`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_memory_operand_contract_test.cpp`

## Current Targets

- Primary external representative: `c_testsuite_aarch64_backend_src_00173_c`
- Current first bad fact: `char *a = "hello"` stores `%t2` into `%lv.a`,
  but `%t2` is published as `sp+72` rather than the `.str0` address.
- Primary local pointer publication contract: a string literal or global data
  address flowing into a local pointer must materialize the global address as
  the pointer value before later dynamic pointer consumers run.
- Stability contracts:
  `backend_lir_to_bir_notes`,
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_memory_operand_contract`,
  and focused tests added for this publication path.
- Idea 356 stability contract: dynamic pointer-derived byte loads for `*b`,
  `*src`, or equivalent consumers must remain dynamic loads through the
  current pointer value.

## Non-Goals

- Do not reopen semantic-BIR dynamic pointer-derived byte-load preservation
  from idea 356 except to keep that behavior stable.
- Do not reopen direct external-call string/global argument lowering from idea
  287 unless fresh evidence proves the same owner and the route is recorded in
  `todo.md`.
- Do not reopen AArch64 address-valued memory or call-argument publication
  from idea 355 without fresh first-bad-fact evidence.
- Do not handle same-module recursive pointer-formal/callee-saved-home
  publication for `00181`.
- Do not handle frontend admission work for `00005`.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, unrelated AArch64 call publication work, runner behavior,
  timeout policy, CTest registration, proof-log handling, expectation changes,
  or unsupported classification changes.

## Working Model

Idea 356 repaired the semantic bug where dynamic pointer-derived byte loads
from string data collapsed to fixed global-byte loads. After that repair,
prepared addressing records the later `*b` and `*src` loads as
`base=pointer_value`, so those consumers now follow the current pointer value.

The remaining `00173` failure is earlier. The source assignment
`char *a = "hello"` lowers to a local pointer store, but the producer carrier
for the string literal address never becomes the `.str0` pointer value.
Prepared BIR treats the carrier as an ordinary frame spill
(`slot#26+stack72`), and generated AArch64 emits
`add x9, sp, #72; str x9, [sp, #16]`. Runtime consumers correctly follow that
dynamic pointer, but the pointer value is invalid, causing the current
segmentation fault.

The likely repair is in the path that represents, prepares, allocates, or
prints global/string addresses when they are values being stored to pointer
locals. The route must keep legitimate fixed global-byte loads separate from
address-valued pointer publication.

## Execution Rules

- Start with read-only localization across source, semantic BIR, prepared BIR,
  and generated AArch64 before editing.
- Record the expected `.str0` or global-data pointer value, the actual
  stack-spill publication, the producer carrier, the local pointer store, and
  the first runtime consumer in `todo.md`.
- Add focused coverage for the pointer-value publication contract before or
  with the repair.
- Keep dynamic pointer-derived byte loads dynamic; do not restore fixed
  `LoadGlobalInst @.str0` byte loads for `*b`, `*src`, or similar consumers.
- Preserve direct fixed global/string byte-load behavior when the source
  expression is actually a fixed global byte access.
- Treat filename matching, literal matching, stack-offset matching, runner
  changes, timeout changes, expectation weakening, and CTest-registration
  changes as route failure.
- If the repair exposes a later failure, classify the new first bad fact in
  `todo.md` before requesting closure or a lifecycle split.

## Step 1: Localize The Pointer-Value Publication Boundary

Goal: identify the exact boundary where the string literal address carrier
loses its global-symbol address and becomes a frame-spill address.

Actions:

- Reproduce or inspect existing focused evidence for `00173` around
  `char *a = "hello"`, `%t2`, `%lv.a`, prepared storage, generated AArch64,
  and the first `printf("%s\n", a)` runtime consumer.
- Compare semantic BIR, prepared BIR, and generated AArch64 for the pointer
  value path before later `*a`, `*b`, and `*src` dynamic loads.
- Identify whether the owner is semantic BIR address materialization,
  prepared address/provenance classification, prealloc pointer-carrier
  storage, spill/reload handling, or AArch64 global-address codegen.
- Check at least one nearby string/global pointer-value shape so the owner is
  not only `00173`.

Completion check:

- `todo.md` records the first bad publication boundary, expected literal or
  global address, actual stack-spill address, producer carrier, local pointer
  store, first runtime consumer, and owner classification.
- The evidence distinguishes pointer-address publication from the dynamic
  byte-load preservation already repaired by idea 356.

## Step 2: Add Focused Pointer-Publication Coverage

Goal: lock down a string/global address flowing as a pointer value into a
local pointer and then into a dynamic pointer consumer.

Actions:

- Add or update the narrowest backend, prepared-route, or semantic test that
  observes address-valued pointer publication before runtime failure.
- Assert that `char *p = "literal"` or an equivalent global-data pointer case
  publishes the global address as the pointer value for local storage.
- Include a dynamic consumer check so the test proves later loads use the
  stored pointer value, not a testcase-shaped direct global-byte shortcut.
- Preserve focused idea 356 coverage for dynamic pointer-derived byte loads.

Completion check:

- Focused coverage fails on the current stack-spill pointer publication and
  names the global-address pointer-value contract clearly.
- Adjacent fixed global/string byte-load and idea 356 dynamic-load contracts
  remain covered.

## Step 3: Repair String/Global Address Pointer Publication

Goal: make the localized owner publish the literal/global address as the
pointer value before local storage and prepared/backend consumers use it.

Actions:

- Update only the localized materialization, provenance, prealloc, or AArch64
  codegen path needed for the classified owner.
- Ensure local pointer stores receive the global/string address value rather
  than the address of a frame spill for the producer carrier.
- Keep direct fixed global-byte loads and dynamic pointer-derived byte loads
  distinct.
- Re-run the focused coverage from Step 2 and the idea 356 stability subset.

Completion check:

- Focused coverage proves the string/global address is published as a pointer
  value for local pointer storage.
- Generated or prepared evidence no longer stores `sp+offset` as the literal
  pointer value for the representative path.
- `git diff --check` passes.

## Step 4: Prove Representative Progress And Classify Remainders

Goal: prove `00173` advances beyond the current segmentation fault or passes,
while preserving nearby backend contracts.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00173_c)$' | tee test_after.log`
- Add any focused tests touched in Steps 2 and 3 to the executor proof command.
- If `00173` still fails, classify the new first bad fact by semantic BIR,
  prepared BIR, generated AArch64, or runtime evidence.
- Split a new source idea only if the remaining failure is outside
  string/global address pointer-value publication.

Completion check:

- The delegated proof command and result are recorded in `todo.md`.
- `00173` advances beyond the current segmentation fault or passes without
  expectation, timeout, runner, CTest-registration, proof-log, or
  filename-specific changes.
- Focused idea 356 coverage remains stable, proving the dynamic pointer-load
  repair was not undone.
