# Backend Ready-IR Contract Todo

Status: Active
Source Idea: ideas/open/35_backend_ready_ir_contract_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Define the backend-owned IR model.
- Exact target for the next iteration: extend `src/backend/ir.*` past the
  newly lowered predecessor-local conditional-join slice into a bounded join
  where the merge still stays explicit but at least one predecessor computes
  its incoming scalar with a second typed ALU form beyond add-immediate, so
  the backend-owned join contract is not special-cased to one arithmetic shape.
- Resume notes:
  - backend-owned IR now lowers a bounded four-block
    compare/branch/join/return slice where `then` and `else` each compute one
    typed `i32 add` before branching to the merge, and the join `phi` merges
    those predecessor-local SSA values instead of only immediate constants
  - both x86 and AArch64 now accept that predecessor-computed conditional-join
    slice directly from explicit backend IR inputs instead of falling back to
    backend-IR text or legacy LIR
  - the next highest-value join seam is widening predecessor-local merge input
    computation beyond add-immediate without reopening raw adapter text
  - backend-owned IR now lowers the bounded four-block
    compare/branch/join/return shape into an explicit join block with a typed
    `phi` result instead of stopping at branch-to-return control flow
  - both x86 and AArch64 now accept that lowered conditional-join slice
    directly from explicit backend IR inputs instead of routing through legacy
    LIR fallback or backend-IR text
  - the next highest-value control-flow seam is a join that merges computed
    scalar state instead of immediate constants; broader general CFG lowering
    still remains deferred
  - bounded while-countdown loops with a single loop-carried scalar now lower
    into explicit backend-owned loop IR with a `phi` state, compare, backedge,
    and exit block instead of being evaluated away into a constant return
  - both x86 and AArch64 now accept that lowered explicit backend-IR countdown
    loop directly at the emitter seam instead of falling back to backend-IR
    text or legacy LIR for the supported while-loop slice
  - the next highest-value control-flow seam is an explicit join/merge shape;
    do-while countdowns and other branch merges still remain outside the
    lowered backend-owned control-flow slice
  - backend-owned IR now carries an explicit compare instruction plus
    conditional-branch terminator, and the bounded three-block
    compare/branch/return slice lowers directly into backend-owned IR instead
    of remaining legacy-LIR-only in backend emitters
  - both x86 and AArch64 now accept that lowered conditional-return slice
    directly from explicit backend IR inputs instead of routing through legacy
    LIR fallback
  - the next highest-value control-flow seam is no longer terminal compare and
    return; the follow-on slice should introduce an explicit join or loop
    branch pattern that still depends on legacy block wiring today
  - backend-owned scalar global stores now have an explicit instruction form in
    `src/backend/ir.hpp`, and the bounded `store @global; load @global; ret`
    slice lowers directly into backend-owned IR instead of preserving raw LIR
    memory text
  - both x86 and AArch64 now accept that lowered scalar global store-reload
    slice directly from explicit backend IR inputs instead of routing through
    legacy LIR fallback
  - the next highest-value seam is multi-block control flow; broader memory-op
    support no longer blocks another bounded explicit-backend-IR slice
  - backend-owned string constants now live alongside globals in
    `src/backend/ir.hpp`, and widened byte loads can model the minimal
    string-literal char slice without preserving legacy GEP/cast text
  - both x86 and AArch64 now accept that lowered string-literal slice directly
    from explicit backend IR inputs instead of routing through legacy LIR
  - the next highest-value seam is broader memory-op lowering; multi-block
    control-flow still remains deferred after that
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
  - bounded scalar global pointer round-trips now lower directly into
    backend-owned global-load IR, so explicit backend IR inputs can emit that
    slice without `ptrtoint` / `inttoptr` / alloca fallback text
  - bounded global char/int pointer-difference slices now lower into a
    backend-owned `ptrdiff_eq` compare form that both x86 and AArch64 can emit
    directly from explicit backend IR inputs without legacy LIR fallback
  - string-pool entities, single-block global load/store seams, and bounded
    pointer-difference slices now live inside the current backend-owned IR
    model; multi-block control flow still remains deferred

## Completed Items

- Completed an additional Step 2 predecessor-computed join slice:
  - taught `src/backend/lir_adapter.cpp` to lower the bounded
    compare/branch/join/return shape when `then` and `else` each compute one
    typed `i32 add` before branching, instead of only accepting immediate phi
    inputs at the merge
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit predecessor-local computed phi inputs directly before the join label
  - added focused backend tests covering lowered predecessor-computed join
    printing, validation, x86 explicit-backend-IR emission, and AArch64
    explicit-backend-IR emission
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching the current recorded runbook baseline from
  `test_fail_after.log` with an identical failing-test set and no newly
  failing tests.
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
- Completed an additional Step 2 backend-global slice:
  - extended `src/backend/ir.hpp` with backend-owned global entities plus a
    narrow load/address form and taught the printer/validator to cover them
  - taught `src/backend/lir_adapter.cpp` to lower scalar global loads, extern
    scalar global loads, and extern global-array element loads into backend IR
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered global-load slices without falling back to raw LIR text
  - added focused backend tests covering lowered global rendering/validation
    plus explicit backend-IR emission for x86 and AArch64 global-load seams
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), improving the recorded baseline by 5 passing tests with no
  newly failing tests.
- Completed an additional Step 2 scalar global pointer round-trip slice:
  - taught `src/backend/lir_adapter.cpp` to canonicalize the bounded
    `ptrtoint -> spill/reload -> inttoptr -> spill/reload -> load` global
    round-trip into backend-owned `BackendLoadInst`
  - added focused backend tests covering lowered round-trip IR printing,
    validation, and explicit backend-IR emission on x86 and AArch64
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching the current recorded runbook baseline with no
  pass-count regression.
- Completed an additional Step 2 scalar global store-reload slice:
  - extended `src/backend/ir.hpp` with a backend-owned `BackendStoreInst`
    contract plus printer/validator coverage in
    `src/backend/ir_printer.cpp` and `src/backend/ir_validate.cpp`
  - taught `src/backend/lir_adapter.cpp` to lower the bounded scalar
    `store @global; load @global; ret` shape into backend-owned store/load IR
    instead of leaving the store-bearing memory seam in legacy LIR only
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered scalar global store-reload slice directly
  - added focused backend tests covering lowered store-reload printing,
    validation, x86 explicit-backend-IR emission, x86 direct asm emission,
    AArch64 explicit-backend-IR emission, and AArch64 direct asm emission
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching the current recorded runbook baseline with no
  pass-count regression.
- Completed an additional Step 2 string-pool slice:
  - extended `src/backend/ir.hpp` with backend-owned string constants plus a
    widened load contract so lowered backend IR can represent the minimal
    string-literal char path without preserving legacy cast/GEP scaffolding
  - taught `src/backend/lir_adapter.cpp` to lower `module.string_pool` and the
    bounded string-literal char function shape into backend-owned string
    constants plus an explicit widened byte load instead of rejecting string
    constants outright
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered string-literal char slice directly without legacy LIR
    fallback
  - added focused backend tests covering lowered string-pool printing,
    validation, and explicit backend-IR emission on x86 and AArch64
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching the current recorded runbook baseline with no
  pass-count regression.
- Completed an additional Step 2 global pointer-difference slice:
  - extended `src/backend/ir.hpp` with a backend-owned `BackendPtrDiffEqInst`
    contract plus printer/validator coverage in `src/backend/ir_printer.cpp`
    and `src/backend/ir_validate.cpp`
  - taught `src/backend/lir_adapter.cpp` to canonicalize the bounded global
    char/int pointer-difference slices into the backend-owned compare form
    instead of preserving `ptrtoint` / subtraction / scaling text
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered char/int pointer-difference slices directly
  - added focused backend tests covering lowered pointer-difference IR
    printing, validation, and explicit backend-IR emission on x86 and AArch64
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching the current recorded runbook baseline with no
  pass-count regression.
- Completed an additional Step 2 multi-block control-flow slice:
  - extended `src/backend/ir.hpp` with a backend-owned compare instruction and
    conditional-branch terminator contract so backend IR can represent the
    bounded three-block compare/branch/return seam directly
  - updated `src/backend/ir_printer.cpp` and `src/backend/ir_validate.cpp` to
    render and validate explicit backend-owned conditional control flow
  - taught `src/backend/lir_adapter.cpp` to lower the bounded
    compare/branch/return shape into backend-owned IR instead of leaving it on
    legacy LIR-only emitter pattern matching
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered conditional-return slice directly
  - added focused backend tests covering lowered conditional-return printing,
    validation, x86 explicit-backend-IR emission, and AArch64
    explicit-backend-IR emission
- Verified the current post-change full-suite regression status in
  `test_fail_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), improving the checked `test_fail_before.log` baseline
  (`189` failing / `2371` passing) by 6 passing tests with no newly failing
  tests.
- Completed an additional Step 2 explicit loop-backedge slice:
  - extended `src/backend/ir.hpp` with a backend-owned `phi` instruction so
    loop-carried backend IR state can be represented explicitly instead of
    being hidden behind adapter evaluation
  - taught `src/backend/lir_adapter.cpp` to lower the bounded
    while-countdown local-scalar shape into explicit backend-owned loop IR
    with entry, loop header, body, and exit blocks
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered countdown-loop backedge slice directly
  - added focused backend tests covering lowered countdown-loop printing,
    validation, x86 explicit-backend-IR emission, AArch64
    explicit-backend-IR emission, and the x86 direct asm path
- Verified the current post-change full-suite regression status in
  `test_fail_after_step5_countdown_loop.log`: `93% tests passed`, `183 tests
  failed out of 2560` (2377 passing), matching the current recorded runbook
  baseline from `test_fail_after.log` with an identical failing-test set and
  no newly failing tests.
- Completed an additional Step 2 explicit join-form control-flow slice:
  - taught `src/backend/lir_adapter.cpp` to lower the bounded
    compare/branch/join/return shape into explicit backend-owned IR with
    predecessor branches plus a typed join-block `phi`
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered conditional-join slice directly
  - added focused backend tests covering lowered conditional-join printing,
    validation, x86 explicit-backend-IR emission, and AArch64
    explicit-backend-IR emission
- Verified the current post-change full-suite regression status in
  `test_fail_after_conditional_phi_join.log`: `93% tests passed`, `183 tests
  failed out of 2560` (2377 passing), matching the current recorded runbook
  baseline from `test_fail_after.log` with an identical failing-test set and
  no newly failing tests.
- Completed an additional Step 2 computed join-result slice:
  - taught `src/backend/lir_adapter.cpp` to lower the bounded
    compare/branch/join/`phi -> add immediate -> ret` shape into explicit
    backend-owned IR instead of stopping at a bare phi-return join
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so explicit backend IR inputs can
    emit the lowered computed join block directly after materializing the phi
    input on each predecessor
  - added focused backend tests covering lowered computed-join printing,
    validation, x86 explicit-backend-IR emission, and AArch64
    explicit-backend-IR emission
- Verified the current post-change full-suite regression status in
  `test_after.log`: `93% tests passed`, `183 tests failed out of 2560`
  (2377 passing), matching `test_fail_after.log` exactly with an identical
  failing-test set and no newly failing tests.
