# Backend Ready-IR Contract Todo

Status: Active
Source Idea: ideas/open/35_backend_ready_ir_contract_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Define the backend-owned IR model.
- Exact target for the next iteration: extend the new `src/backend/ir.*`
  contract beyond the current minimal binary/call/return subset and start
  moving backend dispatch toward consuming `lower_to_backend_ir(...)`
  directly instead of exposing a raw `LirModule` boundary.
- Resume notes:
  - canonical emitter-facing IR ownership now lives in `src/backend/ir.hpp`
    and is shared by the new printer/validator layers
  - `src/backend/lir_adapter.{hpp,cpp}` is now a transition-only lowering shim
    plus compatibility alias surface (`render_module(...)`)
  - `src/backend/backend.hpp` exposes `lower_to_backend_ir(...)`, but
    `emit_module(...)` still dispatches raw `LirModule` into target emitters
  - the current backend IR still only models the existing minimal binary/call
    instruction subset plus return terminators; globals, externs, memory, and
    multi-block control-flow remain for later slices

## Completed Items

- Activated this runbook from
  `ideas/open/35_backend_ready_ir_contract_plan.md` after closing and archiving
  `ideas/open/38_lir_de_stringify_legacy_safe_refactor_plan.md`.
- Preserved the `38` handoff contract in ideas `35`, `36`, and `37`, including
  the stable typed-LIR helper surface and the remaining adapter compatibility
  seams that the backend-ready IR work must retire.
- Completed the Step 1 audit of the current backend ingress surface:
  the public backend entry still takes `LirModule`, but the effective
  emitter-owned contract is the `Backend*` IR family currently declared in
  `src/backend/lir_adapter.hpp`.
- Landed the first Step 2 backend-owned IR slice:
  - added `src/backend/ir.hpp/.cpp`, `src/backend/ir_printer.hpp/.cpp`, and
    `src/backend/ir_validate.hpp/.cpp`
  - moved the emitter-facing `Backend*` IR type ownership out of
    `src/backend/lir_adapter.hpp`
  - added the explicit `lower_to_backend_ir(...)` API to
    `src/backend/backend.hpp`
  - added focused backend tests covering printer output, validator success, and
    validator rejection of malformed IR
- Verified regression status after a clean rebuild:
  `test_before.log` and `test_after.log` both report `93% tests passed`,
  `188 tests failed out of 2560` (2372 passing), with no pass-count regression.
