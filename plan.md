# LIR To BIR Packed Bitfield Scalar Binop Semantics Runbook

Status: Active
Source Idea: ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md
Activated after: ideas/closed/409_prepared_packed_fp128_global_initializer_admission.md

## Purpose

Repair the semantic LIR-to-BIR scalar-binop boundary exposed after packed
aggregate global initializer admission advanced `20040709-2.c`.

## Goal

Lower the first supportable bitfield-style scalar operation chain over packed
aggregate field storage while preserving width, signedness, truncation, and
packed-field semantics.

## Core Rule

The repair belongs to semantic LIR-to-BIR lowering. Do not patch RV64 object
emission or global initializer admission to compensate for missing semantic
scalar facts.

## Read First

- `ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md`
- `ideas/closed/409_prepared_packed_fp128_global_initializer_admission.md`
- `tests/c/external/gcc_torture/src/20040709-2.c`
- Current proof artifacts under `build/agent_state/409_step3_packed_global_proof/`

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/20040709-2.c`
- Failing function: `fn1A`
- Operation shape: bitfield-style `load i16`, `lshr`, `and`, `zext`, `add`,
  `trunc`, `shl`, `or`, and `store i16` over packed `%struct.A`.

## Non-Goals

- Do not reopen packed aggregate global initializer admission from idea 409.
- Do not implement RV64 object-route scalar compare/trunc or instruction
  fragment lowering from this pre-prepared semantic failure.
- Do not infer semantic scalar facts in RV64 object emission.
- Do not make broad bitfield/frontend rewrites outside the observed semantic
  LIR-to-BIR scalar-binop boundary.
- Do not use filename-specific branches, expectation rewrites, unsupported
  downgrades, or allowlist filtering.

## Working Model

After idea 409, `20040709-2.c` no longer stops on bootstrap global admission.
The object-route compile completes, but prepared dump stops before handoff:

```text
semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family
```

Step 1 should identify the first unsupported scalar-binop operation and the
semantic lowering owner before any implementation starts.

## Execution Rules

- Start with classification proof before implementation.
- Keep each implementation packet tied to one concrete scalar-binop semantic
  family.
- Inspect LIR/HIR/BIR diagnostics before editing semantic lowering.
- Preserve integer width, signedness, extension, truncation, mask, shift, OR,
  and packed-field store semantics.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat target-emitter changes, diagnostic-only churn, expectation rewrites,
  and named-case green proof as route failures.

## Step 1: Classify The Fn1A Scalar Binop Residual

Goal: identify the first unsupported semantic scalar-binop operation in
`fn1A` and name the supportable lowering family.

Actions:

- Reproduce or inspect the fresh `20040709-2.c` prepared-dump failure.
- Record the exact LIR/semantic operation sequence around `fn1A`.
- Locate the semantic LIR-to-BIR code that classifies the failure as
  `scalar-binop semantic family`.
- Decide whether the first repair packet is logical right shift, mask/and,
  zero-extension, truncation, shift-left, OR, or a composed bitfield update
  family.
- Route unrelated residuals to lifecycle review instead of expanding this
  idea.

Completion check:

- `todo.md` records the exact first unsupported scalar-binop form, the intended
  semantic lowering file/function area, the representative set, and the
  supervisor-delegated proof command.
- Any non-410 residual is routed with precise evidence.

## Step 2: Lower The First Supported Scalar Binop Family

Goal: implement semantic LIR-to-BIR lowering for the first classified
bitfield-style scalar-binop family.

Actions:

- Update semantic LIR-to-BIR lowering for the selected scalar-binop operation
  or narrow composed family.
- Preserve width, signedness, extension/truncation, and packed-field storage
  semantics.
- Add or update focused semantic/backend coverage for supported and rejected
  adjacent forms.

Completion check:

- `20040709-2.c` advances past the selected `fn1A` scalar-binop failure, or
  focused coverage proves the family with a precise residual owner for the
  representative.
- Existing backend/semantic lowering tests remain green.

## Step 3: Prove Representative And Residual Ownership

Goal: prove the scalar-binop repair advanced 410 without hiding later
prepared, object-route, or runtime residuals.

Actions:

- Run the supervisor-selected `20040709-2.c` object-route compile plus
  prepared dump proof.
- Run the supervisor-selected backend CTest subset.
- If the case advances to another semantic, prepared, RV64 object-route, link,
  qemu, or runtime boundary, classify it separately.

Completion check:

- `todo.md` records exact proof commands, results, and residual ownership.
- No expectation rewrites, unsupported downgrades, allowlist filtering,
  target-emitter inference, or filename-specific fixes are used as acceptance
  evidence.
- The supervisor has enough evidence to continue, request route review, or ask
  the plan owner for close/deactivation handling.
