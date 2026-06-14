Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Proof

# Current Packet

## Just Finished

Step 4 - Focused Proof assessed the available enabled proof for the selected
x86 scalar named-`i32` Route 6 / prepared call argument source agreement path.
No enabled test was added or changed.

Proof rows assessed:

- Agreement path: Step 3 found the x86 path was already wired through
  `find_consumed_scalar_i32_call_argument_source_authority(...)`, which requires
  a prepared call argument plan, a shared Route 6 call-use/source index, named
  `bir::TypeKind::I32`, matching `argument_value`, matching prepared and Route
  6 `source_value_id`, and a present Route 6 `source_value_name`.
- Prepared-only and Route 6-only rows: the helper returns `std::nullopt` unless
  both evidence sources are present, so the enabled proof is compatibility
  preservation rather than a direct x86 rejection-row CTest.
- Mismatch rows: `source_value_id` and Route 6 argument/source matching remain
  required before a source name is accepted; the enabled proof preserves the
  existing stdarg semantic/prepared handoff outputs.
- Unsupported, fallback, and policy-sensitive rows: non-`i32`, missing-name,
  missing-plan, and unsupported shapes keep the existing fallback path by
  helper contract; no ABI/register/stack/result policy, wrapper/fallback name,
  helper/oracle status, public prepared call-plan API, route-debug string,
  expected output, `ConsumedPlans` contract, or riscv behavior changed.
- Duplicate/conflict rows: no enabled CTest in this configured build directly
  exercises x86 duplicate/conflict Route 6 call-source rejection for this
  adapter.

Direct x86 proof limitation:

- `build/CMakeCache.txt` has `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`.
- `ctest --test-dir build -N` registers the delegated prepared/stdarg tests
  but does not register `backend_x86_route_debug` or
  `backend_x86_handoff_boundary` in this configured test set.
- Therefore this packet does not claim those disabled direct x86 tests ran.
  The available enabled proof is sufficient for the Step 4 todo-only focused
  proof record because Step 3 made no code changes and confirmed the adapter was
  already wired, but source-level direct x86 row coverage remains limited by
  the disabled test surface.

## Suggested Next

Execute Step 5 by sweeping compatibility surfaces and deciding whether the
remaining disabled direct x86 proof limitation needs supervisor-selected
configuration work before lifecycle close.

## Watchouts

- Step 3 was already wired in code, and Step 4 did not add tests because the
  direct x86 route-debug / handoff-boundary tests are disabled in the current
  default build configuration.
- Keep the `source_value_name` requirement as part of named authority; missing
  names are fallback rows, not partial successes.
- Route-debug status rows already name `gate=agreed`,
  `gate=missing_source_value`, `gate=missing_source_name`,
  `gate=prepared_source_mismatch`, `gate=source_value_mismatch`, and
  `gate=blocked`; do not rewrite these strings as an implementation shortcut.
- The enabled proof subset preserves prepared printer and stdarg semantic /
  prepared handoff behavior, but it is not a direct substitute for disabled x86
  rejection-row CTests.
- Riscv, ABI/register/stack/result policy, helper selection, wrapper
  instruction text, and fallback names remain out of scope.

## Proof

Command: `(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_printer|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff)$' --output-on-failure) > test_after.log 2>&1`

Result: passed.

Proof log: `test_after.log`.
