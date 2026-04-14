# Reconnect Targets To Prepared BIR Only

Status: Open
Created: 2026-04-14
Derived from: `plan.md` backlog split from `ideas/open/46_backend_reboot_bir_spine.md`

## Why This Idea Exists

Even with semantic lowering and prepare rebuilt, the reboot is not finished
until target backends ingest prepared BIR as their normal contract. This is a
separate integration initiative from the current shared-lowering work.

## Goal

Make x86, aarch64, and riscv64 consume prepared BIR only, with no raw-LIR
fallback path in the normal backend route.

## Primary Targets

- `src/backend/backend.cpp`
- target codegen entry points under `src/backend/*/codegen/`

## Scope

- define target ingestion contracts in terms of prepared BIR
- reconnect x86, aarch64, and riscv64 to that contract
- reject direct/raw LIR fallback paths in the shared backend driver
- expand proving only after prepared-BIR ingestion is real

## Dependencies

- semantic BIR lowering must already cover the needed capability families
- `prepare` must already own legality and phase outputs needed by targets

## Guardrails

- do not recover target support by reintroducing direct LIR routing
- do not hide target contract gaps behind testcase-shaped emission hooks

## Success Condition

This idea is complete when:

- backend routing no longer depends on direct LIR escapes
- target recovery is explained by shared BIR plus prepare capability growth
- each target backend consumes prepared BIR as the default path
