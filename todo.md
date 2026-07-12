# Current Packet

Status: Active
Source Idea Path: ideas/open/721_x86_defined_function_prepared_core_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Missing Prepared-Core Fact

## Just Finished

- Step 1 localized the focused abort to the defined BIR function
  `grouped_spill_reload_contract` (module function index 1, prepared
  `FunctionNameId` 0, `is_declaration == false`, one `entry` block). The name
  resolves consistently at x86 lookup; this is not a stale or mismatched-name
  failure.
- Earliest first bad fact: `prepare_grouped_spill_reload_contract_module()`
  admits the definition to stack layout, liveness, regalloc, addressing, and
  value-location production without ever running `BirPreAlloc::run_legalize()`.
  Consequently no `PreparedControlFlowFunction` is published for function id
  0. `PreparedMirCoreView` binds the BIR definition but, at its common
  three-bank admission check, omits the function entry because control flow is
  null while addressing and value locations are present. x86 emission then
  sees the definition in `defined_functions()` but no `function_view()` and
  correctly fails closed.
- Nearby successful comparison: the immediately preceding
  `grouped_cross_call_preservation_contract` x86 module-emitter check uses
  `prepare_grouped_riscv_module_with_overrides()`, which runs legalization
  before stack layout/liveness/regalloc; that defined function publishes
  control flow and receives a genuine prepared-core view.
- Owning producer seam: common `BirPreAlloc` phase admission/publication for
  control-flow readiness before later independently callable phases, with
  `PreparedMirCoreView`'s all-three-bank admission as the first consumer that
  exposes the omission. This is generic prepared-core completeness within idea
  721, not call-plan cursor or block-entry attribution scope.

## Suggested Next

- Step 2: make the common preparation phase boundary ensure that an eligible
  defined function admitted to later stack/liveness/regalloc production also
  has matching prepared control-flow facts. Keep the repair semantic and
  phase-general; do not inject readiness in this fixture or synthesize facts in
  x86/`PreparedMirCoreView`. Prove both the localized manual-phase route and the
  already-successful full/legalized route retain stable `FunctionNameId`
  agreement.

## Watchouts

- Do not weaken or bypass the x86 prepared-core invariant.
- Do not key behavior to the fixture or function name, treat a definition as a
  declaration, or manually inject readiness.
- Stop for lifecycle review if the first bad fact belongs specifically to idea
  716 call-plan cursor semantics or requires idea 718 scope.
- `PreparedMirCoreView` currently makes no distinction among missing control
  flow, value locations, and addressing when it skips entry construction; Step
  2 should repair the producer boundary without relaxing that fail-closed
  all-three-bank contract.

## Proof

- Ran exactly:
  `cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$' >> test_after.log 2>&1`.
- Build passed; the focused test reproduced the expected abort with
  `x86::module::emit requires prepared core facts for every defined function`.
  This is sufficient localization/reproduction proof for Step 1 and remains a
  failing acceptance test pending Step 2. Canonical log: `test_after.log`.
