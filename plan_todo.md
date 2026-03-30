# Backend Ready-IR Contract Todo

Status: Active
Source Idea: ideas/open/35_backend_ready_ir_contract_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Define the backend-owned IR model.
- Exact target for the next iteration: extend `src/backend/ir.*` past the
  current function-only subset so global definitions and extern global loads
  can be emitted from lowered backend IR without using the optional legacy
  `LirModule` fallback.
- Resume notes:
  - canonical emitter-facing IR ownership now lives in `src/backend/ir.hpp`
    and is shared by the new printer/validator layers
  - `src/backend/lir_adapter.{hpp,cpp}` is now a transition-only lowering shim
    plus compatibility alias surface (`render_module(...)`)
  - `src/backend/backend.hpp` now accepts backend-owned IR at the public emit
    seam, while the `BackendModuleInput{LirModule}` compatibility path stays
    lazy and routes unsupported slices through legacy emitters
  - `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` now accept lowered backend IR
    directly for the current minimal return/direct-call slices and fall back to
    legacy LIR only for still-unlowered target-specific cases
  - lowered backend IR now carries extern declarations as backend declaration
    functions with typed parameter inference from direct call sites
  - globals, string-pool entities, memory ops, and multi-block control-flow
    still remain outside the current backend-owned IR model

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
- Completed a Step 4 lowering-boundary slice:
  - changed `src/backend/backend.hpp/.cpp` so `BackendModuleInput` can carry a
    backend-owned IR module as the primary emit contract, while preserving a
    lazy `LirModule` compatibility constructor for fallback-only callers
  - updated `src/codegen/llvm/llvm_codegen.cpp` to attempt explicit
    `lower_to_backend_ir(...)` before backend emission and fall back cleanly
    when the current lowering shim reports `Unsupported`
  - added x86 and AArch64 emitter overloads that accept lowered backend IR
    directly for the current minimal supported slices and only consult legacy
    LIR when the target backend still needs transitional parsing/regalloc state
  - added focused coverage proving the x86 backend seam accepts explicit
    lowered backend IR without a legacy `LirModule`
- Verified the post-change full-suite regression status in `test_after.log`:
  `93% tests passed`, `188 tests failed out of 2560` (2372 passing), matching
  the prior recorded baseline from this runbook with no newly failing tests.
- Completed an additional Step 2 extern-declaration slice:
  - taught `src/backend/lir_adapter.cpp` to lower `LirModule::extern_decls`
    into backend-owned declaration functions instead of rejecting them
  - infer declaration parameter types from typed direct call sites so lowered
    backend IR prints coherent extern prototypes
  - added focused backend tests covering lowered extern-declaration printing,
    validation, x86 explicit-backend-IR emission, and the updated AArch64
    fallback text expectation
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `188 tests failed out of 2560`
  (2372 passing), matching the previously recorded runbook baseline with no
  pass-count regression.
