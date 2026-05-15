Status: Active
Source Idea Path: ideas/open/247_explicit_got_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Explicit GOT Policy Into Prepared Facts

# Current Packet

## Just Finished

Step 2 published explicit GOT-required address-materialization policy carriers
into target/BIR/prepared facts without selecting or printing GOT.

- Added `TargetRelocationModel` to `TargetProfile`, wired `-fPIC`/`-fpic` to
  `Pic`, `-fPIE`/`-fpie` to `Pie`, and rejected mixed PIC plus PIE inputs.
- Added `GlobalAddressMaterializationPolicy` with `Unspecified`, `Direct`, and
  `GotRequired` to BIR global/function symbol metadata.
- Passed `TargetProfile` into global lowering so static globals lower as
  `Direct`, same-module internal PIC/PIE globals lower as `Direct`, and PIC/PIE
  globals without real per-symbol policy remain `Unspecified`.
- Extended prepared address materialization to preserve/dump the policy,
  convert explicit `GotRequired` globals into `PreparedAddressMaterializationKind::GotGlobal`,
  and diagnose unresolved PIC/PIE policy instead of inferring GOT from spelling
  or `is_extern`.
- Added focused prepared tests proving explicit `GotRequired` carrier
  preservation and PIC unresolved-policy diagnostics.

## Suggested Next

Step 3 implementation packet: consume prepared `GotGlobal` carriers in AArch64
selection records without terminal GOT printer output.

## Watchouts

- This packet intentionally does not infer `GotRequired` from external linkage;
  PIC/PIE externs without a real resolved policy remain `Unspecified` and emit
  a prepared diagnostic.
- BIR still lacks full visibility/preemptibility/import metadata. Future policy
  resolution should populate the explicit BIR policy field instead of replacing
  it with name-shaped shortcuts.
- `GotRequired` for TLS globals is explicitly deferred and diagnosed in
  prepared facts until the TLS/GOT interaction is specified.
- `src/backend/bir/lir_to_bir/module.cpp` changed only as the required call
  site to pass `TargetProfile` into owned global lowering.
- Terminal GOT printer output and AArch64 GOT selection were left untouched.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, 139/139 backend tests green. Proof log: `test_after.log`.
