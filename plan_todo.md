# Backend Ready-IR Contract Todo

Status: Active
Source Idea: ideas/open/35_backend_ready_ir_contract_plan.md
Source Plan: plan.md

## Active Item

- Step 1: Audit the current backend ingress contract.
- Exact target for the next iteration: inventory the current adapter-owned
  backend abstractions and record which emitter/backend call paths still depend
  on transition-only helper seams inherited from `38`.
- Iteration focus: read `src/backend/lir_adapter.{hpp,cpp}`,
  `src/backend/backend.cpp`, and the x86/AArch64 emitter entry points, then
  classify which semantics already have a stable typed-LIR input and which must
  become backend-owned IR entities.
- Next iteration's slice: write the audit findings into `plan_todo.md` and
  identify the minimum backend IR entity set required for the first `ir.hpp`
  pass.
- Exact target for the next iteration after this slice: start Step 2 by
  introducing the narrowest compilable backend IR model plus focused tests.

## Completed Items

- Activated this runbook from
  `ideas/open/35_backend_ready_ir_contract_plan.md` after closing and archiving
  `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.
- Preserved the `38` handoff contract in ideas `35`, `36`, and `37`, including
  the stable typed-LIR helper surface and the remaining adapter compatibility
  seams that the backend-ready IR work must retire.
