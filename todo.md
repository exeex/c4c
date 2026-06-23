Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local Effective Address Materialization

# Current Packet

## Just Finished

Step 3 implementation packet repaired the selected local frame-slot address
publication boundary for RV64 prepared emission.

The local-slot lowering path now publishes local pointer-slot address values for
RV64, so pointer locals assigned from local array/subobject addresses get an
explicit prepared pointer add and prepared frame-slot address materialization.
The RV64 prepared pointer-add emitter consumes that metadata and emits a real
`sp + offset` pointer value before the pointer-local store. The focused route
tests were flipped from expected-fail to ordinary passing tests.

The same packet added bounded scalar publication needed by the focused base
array case to reach its return tail: simple i32 binary results can now be
stored into stack-slot homes, and register-result binary ops can materialize
stack-slot operands before the arithmetic op.

Runtime probe movement under
`build/rv64_c_testsuite_probe_latest/triage_312_step3/`: all five probed cases
now emit and link. `00072` passes qemu. `00032` reaches qemu and exits 139;
`00130`, `00005`, and `00143` reach qemu and exit 132.

## Suggested Next

Repair the next coherent local array access boundary exposed by the Step 3
probe: indexed/reassigned local pointer dereference for `00032`, or split to
the byte/subobject tail for `00130` if the supervisor wants to keep array-base
and byte-lane work separate.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- `00072` is the clean end-to-end proof for this local address publication
  slice.
- `00032` no longer fails to publish the base pointer, but still crashes after
  multiple pointer-local reassignments and dereferences.
- `00130` now publishes the constant subobject address and links; its qemu 132
  residual appears beyond the first address-publication boundary.
- `00143` remains too broad for this packet; keep it as indexed local
  array/select-update-chain follow-up evidence.

## Proof

Proof run:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_prepared_local_array_(base|subobject)_pointer'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Results:

- Build passed.
- Focused local-array pointer subset passed `4/4` with no xfail properties.
- Backend subset produced `test_after.log`: `passed=223 failed=1 total=224`.
  The only failing test is the pre-existing
  `backend_riscv_prepared_edge_publication`.
- Strict monotonic guard reported no new failures but failed because the pass
  count did not strictly increase from the current `test_before.log`.
  Non-decreasing guard passed with `delta passed=0 failed=0`.
- Runtime probe results are in
  `build/rv64_c_testsuite_probe_latest/triage_312_step3/probe_results.tsv`.
