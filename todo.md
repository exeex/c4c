Status: Active
Source Idea Path: ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure or Handoff Check

# Current Packet

## Just Finished

Step 2 implemented the first safe RV64 object-route byval aggregate
parameter-home class: a default-address-space byval formal whose prepared value
home is a stack slot backed by a matching permanent `source_kind=byval_param`
frame object. Object emission now admits that fact-complete home and lowers
prepared pointer-value local accesses through the byval parameter value back to
the same frame slot when the published size/align/range facts are in bounds.
Focused RV64 object-emission coverage now builds the admitted byval stack-slot
parameter home and rejects missing/inconsistent byval home facts plus malformed
byval pointer accesses.

## Suggested Next

Ask the plan owner to perform Step 4 closure or handoff review for idea 370.
The representative now advances past `unsupported_byval_param_home` and stops
at `unsupported_local_memory_access` for prepared aggregate/global local-memory
addressing, which is distinct from the byval aggregate parameter-home owner.

## Watchouts

The repair is intentionally narrow: it does not infer aggregate bytes from ABI
stack layout, and it does not admit byval homes without a matching prepared
stack object, frame slot, value home size/align, and in-bounds prepared memory
access. The delegated proof remains nonzero because the selected CTest subset
still includes the pre-existing `riscv64_byval_preserved_pointer_args` failures,
and the representative now exposes a distinct local-memory addressing boundary.

## Proof

Commands run:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
- Delegated proof command exactly as provided by the supervisor, writing
  `test_after.log`

Result: build passed and focused `backend_riscv_object_emission` passed. The
delegated proof wrote `test_after.log` but exited nonzero: the byval CTest
subset still fails `backend_codegen_route_riscv64_byval_preserved_pointer_args`
and `backend_rv64_runtime_riscv64_byval_preserved_pointer_args`, and
`src/20030914-2.c` now fails later with
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing` instead of
`unsupported_byval_param_home`. Supervisor monotonic comparison of the
delegated proof logs passed with no new CTest failures, and a broader
`ctest --test-dir build -j --output-on-failure -R '^backend_'` run after the
patch showed only the same two pre-existing byval preserved-pointer failures.
