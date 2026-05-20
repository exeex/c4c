# Semantic BIR Loop-Carried Pointer Dereference Provenance Runbook

Status: Active
Source Idea: ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md
Activated From: ideas/closed/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md residual split

## Purpose

Repair semantic BIR lowering for dereferences through loop-carried local
pointers initialized from array bases.

## Goal

Make `*from++` and `*to++` style dereferences consume the current loop-carried
pointer-local state instead of freezing to direct array-base slots across
loopback iterations.

## Core Rule

Fix semantic pointer provenance or memory lowering generally. Do not
special-case `00143`, Duff-device blocks, `%lv.from`, `%lv.to`, `%lv.a.0`,
`%lv.b.0`, block numbers, stack offsets, source lines, or emitted instruction
strings.

## Read First

- `ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md`
- `ideas/closed/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md`
  completion note for split evidence
- `ideas/closed/342_aarch64_duff_do_while_latch_condition_emission.md`
  completion note for latch evidence
- Current residual evidence:
  - idea 343 repaired the fixed-offset skip; generated AArch64 now emits
    consecutive short-copy offsets for the Duff fallthrough copy chain
  - idea 342's Duff latch repair remains intact
  - semantic BIR for `00143` updates `%lv.from` and `%lv.to`, but dereferences
    still read and write direct `%lv.a.0` and `%lv.b.0` base slots
  - on loopback, the stored pointer-local updates are not consumed, so later
    iterations repeat `a[0..7] -> b[0..7]` instead of copying the advanced
    source range

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-semantic-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
- Likely owner probes:
  - `src/backend/bir/lir_to_bir/memory/addressing.cpp`
    `lower_memory_gep_inst`
  - `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
    `try_lower_local_slot_pointer_gep`
  - `src/backend/bir/lir_to_bir/memory/provenance.cpp` pointer-provenance
    load/store helpers
- Focused coverage should exercise a local pointer initialized from an array
  base, incremented across a loopback, and dereferenced after the loop-carried
  update.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not reopen the fixed-offset skip repaired by idea 343 unless fresh
  evidence shows generated AArch64 again skips consecutive prepared copy
  offsets.
- Do not reopen Duff latch-condition decrement/compare emission unless fresh
  evidence shows the duplicate latch decrement returned.
- Do not reopen scalar-cast source-publication work unless the old structured
  register-source diagnostic returns.
- Do not broaden into frame layout, aggregate, variadic, parser, semantic HIR,
  or frontend rewrites without fresh first-bad-fact evidence.

## Working Model

Semantic BIR currently recognizes the pointer updates for Duff copy locals, but
the load/store dereference path keeps using fixed local-slot provenance from
the original array base. That is correct only while the pointer is known fixed.
Once the pointer local is updated and reaches a loopback, dereference lowering
must use the current pointer value or carry dynamic provenance instead of
reusing the original base slot.

## Execution Rules

- Keep routine localization and proof notes in `todo.md`.
- Add focused coverage before or with the repair when the loop-carried pointer
  dereference can be expressed without relying only on `00143`.
- Preserve idea 343's corrected fixed-offset fallthrough emission and idea
  342's corrected latch emission.
- Reclassify any remaining `00143` residual by prepared-state,
  generated-code, assembler/linker output, or runtime evidence before claiming
  completion.

## Steps

### Step 1: Localize Loop-Carried Pointer Dereference Boundary

Goal: identify where semantic BIR dereference lowering stops consuming the
current `%lv.from` and `%lv.to` pointer-local values and freezes accesses to
direct `%lv.a.0` and `%lv.b.0` base slots.

Primary target: the `00143` Duff copy loop where pointer locals are incremented
across loopback but later dereferences repeat the first eight array elements.

Actions:

- Reproduce the `00143` runtime residual and regenerate semantic BIR,
  prepared BIR, and AArch64 probes as needed.
- Trace `*from++` and `*to++` from frontend/LIR memory operations into semantic
  BIR loads, stores, local GEP lowering, and pointer-provenance decisions.
- Identify whether the bad direct-slot dereference is caused by local-slot
  pointer GEP folding, stale provenance, load/store helper selection, or
  another concrete semantic BIR boundary.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete semantic BIR memory, local GEP, provenance, or
  dereference-lowering boundary where loop-carried pointer state is first lost.

### Step 2: Repair Pointer-Local Dereference Lowering

Goal: make dereferences through loop-carried local pointers use the current
pointer state without breaking proven fixed local-slot accesses.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the semantic repair without matching `00143`, local names, block
  numbers, stack offsets, source lines, or instruction strings.
- Preserve direct fixed-offset local-slot lowering where provenance is still
  fixed and safe.
- Add or update focused coverage for a pointer local initialized from an array
  base, advanced across loopback, and dereferenced through load and store
  paths.
- Preserve the fixed-offset fallthrough emission behavior from idea 343 and
  the do-while latch condition behavior from idea 342.

Completion check:

- Focused coverage proves loop-carried pointer-local dereferences consume the
  current pointer state after loopback, while fixed local-slot dereferences
  remain supported.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the semantic pointer dereference repair and classify any new first
bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm semantic BIR for the Duff copy loop no longer freezes loop-carried
  `*from++` and `*to++` dereferences to repeated direct base slots.
- Confirm generated AArch64 still includes the consecutive fixed-offset copy
  chain from idea 343 and the single post-decrement latch branch from idea 342.
- If `00143` still fails, classify the new first bad fact from prepared BIR,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside semantic BIR loop-carried pointer dereference lowering.
