Status: Active
Source Idea Path: ideas/open/374_rv64_object_route_non_register_param_homes.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Add Focused Parameter-Home Coverage / Lower Supported Non-Register Parameter Home Shape

# Current Packet

## Just Finished

Steps 2 and 3 implemented the first supported RV64 non-register formal
parameter-home shape.

The RV64 object-route parameter-home gate now admits only scalar GPR formal
stack-slot homes when the prepared value home, ABI integer register ownership,
frame slot, stack object, type, size/alignment, final frame bound, and signed
12-bit stack offset are coherent. Varargs and sret marker formals are skipped
as no-publication formals. Byval aggregate homes remain on their existing
permanent-home-slot path, and ambiguous or unsupported stack-slot formals still
fail closed.

The object emitter now emits encoded formal-entry stack stores for admitted
scalar GPR stack-slot homes. The focused fixture mirrors the audited `%p.fmt`
shape with slot `#11`, offset `64`, size/align `4`, final frame size `80`, and
incoming `a0`, and verifies the emitted `addi sp, sp, -80; sw a0, 64(sp);
addi sp, sp, 80; ret` sequence. Negative fixture mutations cover mismatched
home offsets, frame-slot size, stack-object source/flags, non-GPR ABI class,
out-of-frame homes, and incoherent large offsets.

The expected RV64 variadic aggregate object-failure diagnostic advanced from
the old `unsupported_param_home` gate to the later
`unsupported_variadic_helper_lowering` payload-write-address gate, matching the
new fail-closed boundary.

## Suggested Next

Inspect the next RV64 object-route boundary exposed after scalar GPR formal
stack-slot homes: `va-arg-13.c` now passes the parameter-home gate and reaches a
later `unsupported_instruction_fragment` diagnostic. Keep aggregate `va_arg`
payload-write-address lowering as a separate route unless the supervisor
chooses that helper gate as the next packet.

## Watchouts

- Do not infer parameter layout from source syntax, testcase names, raw BIR
  spelling, or assumed RV64 stack layouts.
- The object-route formal-entry source register is derived from the count of
  preceding integer ABI register formals, not raw parameter index.
- The new scalar stack-slot home support is intentionally strict about
  `regalloc.spill_slot` objects with no address exposure or permanent-home
  flags; byval homes keep the separate byval diagnostic.
- `tests/c/external/gcc_torture/src/va-arg-13.c` still does not build to an
  object; it now stops later with `unsupported_instruction_fragment`.

## Proof

Supervisor-selected proof passed:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Focused pre-proof check also passed:
`cmake --build --preset default --target backend_riscv_object_emission_test &&
ctest --test-dir build -j --output-on-failure -R
'^backend_riscv_object_emission$'`.

Canonical proof log: `test_after.log`.
