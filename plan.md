# AArch64 Scalar Select Result Publication Runbook

Status: Active
Source Idea: ideas/open/345_aarch64_scalar_select_result_publication.md
Activated From: ideas/closed/344_semantic_bir_loop_carried_pointer_deref_provenance.md residual split

## Purpose

Repair AArch64 scalar select result publication so stack-homed select results
are materialized before later store-local or consumer reloads use their homes.

## Goal

Make scalar `csel` results available from their modeled stack homes before any
later store-local or consumer path reloads those homes.

## Core Rule

Fix scalar select result publication generally. Do not special-case `00143`,
`%t9.store0`, `%t13.store0`, `%lv.a.0`, one stack offset, one block number,
one source line, or one exact emitted instruction sequence.

## Read First

- `ideas/open/345_aarch64_scalar_select_result_publication.md`
- `ideas/closed/344_semantic_bir_loop_carried_pointer_deref_provenance.md`
  completion note for pointer-local dereference evidence
- `ideas/closed/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md`
  completion note for fixed-offset fallthrough evidence
- `ideas/closed/342_aarch64_duff_do_while_latch_condition_emission.md`
  completion note for latch evidence
- Current residual evidence:
  - semantic and prepared BIR now load `%lv.from` and `%lv.to`, advance those
    pointer values, and dereference through current pointer-value addresses
  - generated AArch64 computes short integer select results with `csel`
  - generated AArch64 then reloads the modeled select-result home before the
    selected value has been published there
  - the reload observes an unpublished stack home instead of the fresh selected
    register value

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --dump-semantic-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
- Likely owner probes:
  - `src/backend/aarch64/` scalar select lowering and value publication paths
  - `lower_scalar_select_publication`
  - store-local lowering paths that reload stack-homed scalar values
- Focused coverage should exercise a short integer scalar select result that
  feeds a local array store or equivalent stack-homed consumer.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not reopen semantic BIR loop-carried pointer dereference provenance
  repaired by idea 344 unless fresh evidence shows that old first bad fact
  returned.
- Do not reopen Duff fixed-offset fallthrough copy emission repaired by idea
  343 unless generated AArch64 again skips consecutive prepared copy offsets.
- Do not reopen Duff do-while latch condition decrement/compare emission
  repaired by idea 342 unless the duplicate latch decrement returned.
- Do not reopen scalar-cast register-source publication repaired by idea 340
  unless the old diagnostic or owner has returned.
- Do not broaden into frame layout, aggregate, variadic, parser, semantic HIR,
  frontend, or unrelated backend rewrites before proving the scalar select
  publication boundary.

## Working Model

Prepared BIR models short integer select results as stack-homed values that
can feed `bir.store_local` and later consumers. The AArch64 path computes the
selected value in a register with `csel`, but the modeled home remains stale.
Any following path that reloads from that home can consume old or uninitialized
data. The repair should publish the selected register value to the modeled home
at the scalar select boundary or at the matching store-local publication
boundary without changing unrelated fixed local-slot behavior.

## Execution Rules

- Keep routine localization, proof, and residual notes in `todo.md`.
- Add focused coverage before or with the repair when the select-publication
  behavior can be expressed without relying only on `00143`.
- Preserve idea 344's pointer-local dereference repair, idea 343's fixed-offset
  fallthrough emission, and idea 342's latch-condition behavior.
- Reclassify any remaining `00143` residual by prepared-state,
  generated-code, assembler/linker output, or runtime evidence before claiming
  completion.
- Treat expectation changes, unsupported downgrades, runner changes, or
  named-case-only matching as route failure, not progress.

## Steps

### Step 1: Localize Scalar Select Result Publication Boundary

Goal: identify where AArch64 lowering computes a scalar `csel` result without
publishing the selected value to the modeled home expected by later reloads.

Primary target: the `00143` prepared BIR and generated AArch64 path where
stack-homed `i16` select results feed local array stores.

Actions:

- Reproduce the `00143` runtime residual and regenerate semantic BIR,
  prepared BIR, and AArch64 probes as needed.
- Trace the short integer select result from prepared BIR through scalar select
  lowering, value publication, and store-local consumer reloads.
- Identify whether the bad reload is caused by `lower_scalar_select_publication`,
  store-local value publication, register-to-home synchronization, or another
  concrete AArch64 backend boundary.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete AArch64 select/result publication or store-local
  publication boundary where the fresh `csel` register value first fails to
  reach the modeled home.

### Step 2: Repair Scalar Select Result Publication

Goal: ensure stack-homed scalar select results are available from their modeled
homes before any later store-local or consumer reloads.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the publication repair without matching `00143`, temporary names,
  block numbers, stack offsets, source lines, or emitted instruction strings.
- Preserve existing fixed local-slot behavior and unrelated scalar publication
  paths.
- Add or update focused coverage for a short integer select result feeding a
  local store or equivalent stack-homed consumer.
- Preserve the pointer-local dereference behavior from idea 344 and the Duff
  emission repairs from ideas 343 and 342.

Completion check:

- Focused coverage proves a stack-homed scalar select result can feed a local
  store or equivalent consumer without reading an unpublished home.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the scalar select result publication repair and classify any new
first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm generated AArch64 publishes each fresh scalar `csel` result before a
  store-local or consumer path reloads the modeled home.
- Confirm semantic BIR still dereferences through current pointer-local values
  rather than stale direct base slots.
- Confirm generated AArch64 still includes the consecutive fixed-offset copy
  chain from idea 343 and the single post-decrement latch branch from idea 342.
- If `00143` still fails, classify the new first bad fact from prepared BIR,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside scalar select result publication.
