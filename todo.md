Status: Active
Source Idea Path: ideas/open/255_phase_f3_prepared_private_pass_context_metadata_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Consumer Inventory

# Current Packet

## Just Finished

Step 1 - Consumer Inventory completed for `PreparedBirModule::route`,
`invariants`, `completed_phases`, and `notes`. `PreparedBirModule::liveness`
remained out of scope.

Field inventory:

- `route`
  - Direct consumers: declared/defaulted in `src/backend/prealloc/module.hpp`;
    `BirPreAlloc` aggregate construction sets `PrepareRoute::SemanticBirShared`;
    `src/backend/prealloc/prepared_printer.cpp` reads `module.route` for the
    `prepared.module ... route=...` header.
  - Indirect consumers: `src/backend/backend.cpp` prepared dump/focus dump
    routes call `prepare::print`; CMake dump snippets and route-debug tests
    assert `route=semantic_bir_shared` output.
  - Classification: construction-only plus printer/status/debug
    compatibility. x86/riscv target code accepts the prepared aggregate but no
    direct `PreparedBirModule::route` target-facing read was found; other
    `.route` hits are call-preservation or BIR route records, not this field.
  - Preliminary state: candidate. No x86/riscv public consumer currently blocks
    demotion, but printer/debug header compatibility must stay byte-stable.

- `invariants`
  - Direct consumers: declared in `src/backend/prealloc/module.hpp`;
    `run_legalize` appends `NoTargetFacingI1`; `run_out_of_ssa` appends
    `NoPhiNodes`; `src/backend/prealloc/prepared_printer.cpp` reads
    `module.invariants` for the `invariants:` section.
  - Indirect consumers: prepared dumps through `src/backend/backend.cpp` and
    printer tests observe stable invariant names via
    `prepared_bir_invariant_name`.
  - Tests: `tests/backend/bir/backend_prepare_phi_materialize_test.cpp`
    directly scans `module.invariants` for `NoPhiNodes` and
    `NoTargetFacingI1`.
  - Classification: pass-context metadata writes, printer/status/debug
    compatibility, and tests. No x86/riscv target-facing direct read was found.
  - Preliminary state: candidate. No target public consumer blocks demotion;
    invariant-name/output compatibility is retained.

- `completed_phases`
  - Direct consumers: declared/default-initialized in
    `src/backend/prealloc/module.hpp`; appended by `run_legalize`,
    `run_stack_layout`, `run_liveness`, `run_out_of_ssa`, and `run_regalloc`;
    `src/backend/prealloc/prepared_printer.cpp` reads it for the
    `completed_phases:` row.
  - Indirect consumers: prepared dump/focus dump routes in
    `src/backend/backend.cpp`; CMake dump snippets assert phase ordering such
    as `legalize stack_layout liveness out_of_ssa regalloc`.
  - Classification: pass-context metadata writes plus printer/status/debug
    compatibility. No tests or x86/riscv target code directly read the field.
  - Preliminary state: candidate. No x86/riscv public consumer blocks
    demotion; phase-row text/order compatibility must be preserved.

- `notes`
  - Direct consumers: declared/default-initialized in
    `src/backend/prealloc/module.hpp`; appended by `BirPreAlloc::note`,
    `run_legalize`, `run_stack_layout`, `run_liveness`, `run_out_of_ssa`, and
    `run_regalloc`; stack-layout publication helpers receive/append through
    `prepared_.notes`; `src/backend/prealloc/prepared_printer.cpp` reads
    `module.notes` for the `notes:` section.
  - Indirect consumers: prepared dump/focus dump routes in
    `src/backend/backend.cpp`; CMake dump snippets assert note rows such as
    `[stack_layout] ...` and `[regalloc] ...`.
  - Tests: `tests/backend/bir/backend_prepare_stack_layout_test.cpp` directly
    scans `prepared.notes` for the unresolved-PIC diagnostic. `lir_to_bir`
    `result.notes` and x86 API lowering-note reads are separate types and do
    not consume `PreparedBirModule::notes`.
  - Classification: pass-context metadata writes, printer/status/debug
    compatibility, and tests. No x86/riscv prepared-module target-facing
    direct read was found.
  - Preliminary state: candidate. No target public consumer blocks demotion;
    absent-note behavior and note text/status rows must remain exact.

## Suggested Next

Proceed to Step 2 by mapping compatibility contracts and focused proof coverage
for the prepared header route text, completed-phase row, invariant names/rows,
notes rows, and absent-note behavior before any accessor or adapter change.

## Watchouts

- Keep `PreparedBirModule::liveness` out of scope.
- Do not treat call-preservation `.route`, BIR route records, or
  `BirLoweringResult::notes` as consumers of the four `PreparedBirModule`
  metadata fields.
- The x86 and riscv prepared emit/debug paths take the aggregate, but Step 1 did
  not find direct public reads of these four fields there. Recheck target files
  before code changes if the accessor shape changes aggregate visibility.
- Tests currently read `invariants` and `notes` directly; these are not
  target-facing blockers, but any demotion slice must update tests through the
  chosen compatibility surface rather than weakening assertions.

## Proof

Analysis-only packet; no build or test proof required by the supervisor.
Validation command: `git diff --check -- todo.md`.
