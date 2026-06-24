Status: Active
Source Idea Path: ideas/open/328_rv64_prepared_edge_publication_module_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair RV64 Prepared Module/Function Scheduling

# Current Packet

## Just Finished

Step 2 `Repair RV64 Prepared Module/Function Scheduling` completed. The RV64
prepared module emitter now appends available shared edge-publication moves
through `append_edge_publication_move_instruction(...)` after a successful
`append_simple_prepared_bir_function_asm(...)` path, so the simple-emitter
branch no longer bypasses prepared publication scheduling.

Step 3 coverage was checked without test edits: the existing focused regression
already inspects final prepared module text for `mv a1, a0` and still proves the
helper depends on shared lookup authority.

## Suggested Next

Supervisor should review and decide whether this slice is ready for acceptance
or whether broader RV64/backend validation is needed beyond the delegated
prepared subset.

## Watchouts

- The implementation consumes `lookups.edge_publications.publications` only; it
  does not scan BIR predecessor/successor shape or special-case fixture names,
  registers, value ids, or output strings.
- `append_edge_publication_move_instruction(...)` return status remains ignored
  as before, preserving the existing fail-closed helper surface without widening
  this packet into diagnostics policy.
- Untracked `ideas/open/329_native_object_emission_umbrella.md` was observed and
  left untouched.

## Proof

Ran:
`set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper|backend_prepared_printer|backend_codegen_route_riscv64_prepared_)') > test_after.log 2>&1`

Result: build completed and the delegated subset passed, 11/11 tests green.
Proof log: `test_after.log`.
