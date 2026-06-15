Status: Active
Source Idea Path: ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Map Preservation Rows

# Current Packet

## Just Finished

Step 2, `Map Preservation Rows`, completed the analysis-only preservation
matrix for `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

Preservation row statuses:

- `satisfied`: live code and current tests/fixtures prove the specific
  preservation behavior.
- `observable-unproven`: live code exposes or carries the surface, but this
  packet found no field-specific proof that would fail if behavior weakened.
- `blocked`: the expected preservation/fail-closed behavior is not implemented
  or not live-proved; docs-only assertions do not satisfy the row.

Field-by-field preservation matrix:

| Field | Surface | Expected behavior that must not weaken | Current supporting proof | Status |
| --- | --- | --- | --- | --- |
| `invariants` | Prepared printer: `prepare::print(...)` directly reads `module.invariants` and emits an `invariants:` block with one `prepared_bir_invariant_name(...)` row per entry (`src/backend/prealloc/prepared_printer.cpp:52-57`). | Preserve field-specific output, row order, spelling for `NoTargetFacingI1` and `NoPhiNodes`, and omission of the section only when the vector is empty. Do not replace with docs-only, route-only, status-only, or private-context output. | Printer fixture directly mutates the public vector and expects exact rendered text for both invariant names (`tests/backend/bir/backend_prepared_printer_test.cpp:2739-2743`, `:2861-2867`). Phi materialization tests directly scan `module.invariants` and check presence/absence behavior (`tests/backend/bir/backend_prepare_phi_materialize_test.cpp:37-38`, `:208-241`). | satisfied |
| `invariants` | Prepared dump wrapper: `dump_generic_prepared_bir(...)` returns `prepare::print(prepared)` for unfocused prepared dumps and filters only the module/control-flow payload before reprinting for focused dumps (`src/backend/backend.cpp:287-343`). | `--dump-prepared-bir` and focused prepared dumps must continue to expose the same invariant rows that `prepare::print(...)` exposes; focus filtering must not silently erase or synthesize invariant metadata. | CLI dump coverage observes the prepared dump path (`tests/backend/bir/CMakeLists.txt:550-559`), and code inspection shows no invariant filtering in `dump_generic_prepared_bir(...)`. No field-specific focused-dump invariant test was found. | observable-unproven |
| `invariants` | Status/handoff gate: AArch64 `validate_prepared_module_handoff(...)` is the only live status-like prepared handoff validator found (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | If invariants are treated as status gates, missing or mismatched required invariants must fail closed instead of allowing target lowering. Docs that name invariants as gates must not be counted as proof by themselves. | AArch64 docs name `invariants` as handoff gate carriers (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-55`, `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md:134`, `:159`), but live validation currently checks only target arch and ABI. No status row reads `module.invariants`. | blocked |
| `invariants` | x86 route debug: `summarize_prepared_module_routes(...)` and `trace_prepared_module_routes(...)` accept the full `PreparedBirModule` and render route diagnostics (`src/backend/mir/x86/debug/debug.cpp:511-524`, `src/backend/mir/x86/codegen/route_debug.hpp:7-23`); `dump_module(...)` dispatches `MirSummary`/`MirTrace` to this route (`src/backend/backend.cpp:1493-1529`). | Debug surfaces must not weaken by dropping the prepared aggregate or by claiming invariant preservation through a renamed/debug-only mirror. If debug begins depending on invariant state, absent/mismatched invariants must be visible or fail closed. | Route debug is live and aggregate-based, but no direct `module.invariants` read or invariant-rendering row was found in the x86 debug path. Existing route-debug tests prove other prepared authorities, not this field. | observable-unproven |
| `completed_phases` | Prepared printer: `prepare::print(...)` directly reads `module.completed_phases` and emits a single `completed_phases:` line in vector order (`src/backend/prealloc/prepared_printer.cpp:44-50`). | Preserve exact label, phase spelling, vector order, spacing, and omission only when the vector is empty. Do not relabel phases through status/oracle names or hide them behind private pass context. | Printer fixture directly sets `{"legalize", "out_of_ssa"}` and expects exact text (`tests/backend/bir/backend_prepared_printer_test.cpp:2729-2732`, `:2852-2858`). CLI dump coverage also expects prepared phase text for a prepared dump case (`tests/backend/bir/CMakeLists.txt:550-559`). | satisfied |
| `completed_phases` | Prepared dump wrapper: `dump_generic_prepared_bir(...)` forwards the aggregate to `prepare::print(...)`, including focused prepared dump cases after filtering (`src/backend/backend.cpp:287-343`). | Dump output must retain completed-phase evidence exactly as the printer renders it; focus filters must not remove, reorder, or invent phase names. | Code inspection shows no completed-phase filtering in focused dumps, and existing CLI prepared dump coverage observes phase text. No focused-dump phase-specific assertion was found. | observable-unproven |
| `completed_phases` | Status/handoff gate: AArch64 handoff validation (`src/backend/mir/aarch64/abi/abi.cpp:870-881`) is the live status-like validator. | Missing required phases, out-of-order or mismatched phase evidence, or absent phase metadata must fail closed before target lowering if phase completion is a status gate. | Docs identify completed phases as required gate carriers (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-55`, `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md:134`, `:159`), but live validation does not inspect `completed_phases`. | blocked |
| `completed_phases` | x86 route debug summary/trace wrappers and `MirSummary`/`MirTrace` dump dispatch (`src/backend/mir/x86/debug/debug.cpp:511-524`, `src/backend/mir/x86/codegen/route_debug.hpp:7-23`, `src/backend/backend.cpp:1493-1529`). | Debug surfaces must keep consuming the prepared aggregate and must not present route status as a substitute for completed-phase evidence. Any future phase-dependent debug behavior needs direct proof. | Route debug accepts the aggregate, but no direct `completed_phases` read or rendered phase row was found in route debug. Existing route-debug coverage is not field-specific for phases. | observable-unproven |
| `notes` | Prepared printer: `prepare::print(...)` directly reads `module.notes` and emits `notes:` rows as `  - [phase] message` (`src/backend/prealloc/prepared_printer.cpp:59-64`). | Preserve label, row format, phase/message text, row order, and omission only when notes are empty. Do not conflate `PreparedBirModule::notes` with separate `BirLoweringResult::notes`. | Printer fixture directly pushes a prepared note and expects exact rendered text (`tests/backend/bir/backend_prepared_printer_test.cpp:2729-2735`, `:2852-2858`). CLI prepared dump coverage observes note text (`tests/backend/bir/CMakeLists.txt:550-559`). | satisfied |
| `notes` | Focused prepared dump filters: function/block/value filters erase selected prepared-note rows before reprinting (`src/backend/backend.cpp:588-613`, `:670-686`) and `dump_generic_prepared_bir(...)` then calls `prepare::print(...)` (`src/backend/backend.cpp:287-343`). | Focused dump filtering must preserve relevant notes and drop only notes outside the requested focus. It must not rewrite notes into generic status text or silently discard all prepared metadata. | Code inspection shows explicit note filtering and reprint behavior. No field-specific test was found that proves note preservation/dropping for function, block, and value focus rows. | observable-unproven |
| `notes` | Status/failure notes distinction: prepared notes are public aggregate metadata, while `BirLoweringResult::notes` are separate lowering failure/status payloads used in backend errors (`src/backend/backend.cpp:105-145`, `src/backend/mir/x86/api/api.cpp:11-22`). | Status or failure-message behavior must not claim prepared-note preservation by reading the separate lowering-note channel. Prepared notes must remain available on prepared dumps and aggregate handoffs. | Live failure-message status code reads lowering notes, not `PreparedBirModule::notes`; prepared-note printer proof is separate. No live status gate was found that consumes prepared notes. | blocked |
| `notes` | x86 route debug summary/trace wrappers and `MirSummary`/`MirTrace` dump dispatch (`src/backend/mir/x86/debug/debug.cpp:511-524`, `src/backend/mir/x86/codegen/route_debug.hpp:7-23`, `src/backend/backend.cpp:1493-1529`). | Debug surfaces must continue to receive the full prepared aggregate and must not replace prepared notes with lowering notes, route labels, or classification-only debug text. If note-dependent debug behavior appears, it needs direct field proof. | Route debug accepts `PreparedBirModule`, but no direct `notes` read or rendered prepared-note row was found in route debug. Existing route-debug tests prove other route authorities, not prepared-note preservation. | observable-unproven |

Blockers carried forward:

- Status preservation is blocked for `invariants` and `completed_phases`
  because AArch64 docs identify them as handoff gates, but the live validator
  only checks target arch/ABI. Docs-only assertions are not proof.
- Status preservation is blocked for `notes` because live failure/status
  message code reads `BirLoweringResult::notes`, not
  `PreparedBirModule::notes`. The two note channels must remain distinct.
- Focused prepared dump preservation for `invariants`, `completed_phases`, and
  `notes` is directly observable from code but unproven by field-specific
  tests.
- x86 route debug carries the prepared aggregate but does not directly read or
  render these three fields; this is observable but unproven field
  preservation, not authority to demote or ignore the metadata.
- No demotion, privatization, wrapper, migration, private-pass-context, or
  implementation work is authorized by this Step 2 matrix.

## Suggested Next

Execute Step 3 from `plan.md`: map fail-closed rows for invalid, mismatched,
missing, direct-payload-read, and absent metadata behavior for each of
`invariants`, `completed_phases`, and `notes`.

## Watchouts

- This active plan is proof-gate work only.
- Do not demote, delete, privatize, wrap, migrate, or implement authority
  movement for `PreparedBirModule` metadata.
- Do not weaken printer, status, debug, helper/oracle, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  status/oracle relabeling, printer/debug formatting changes,
  classification-only notes, or named-fixture proof.
- Proof must cover all three metadata fields and invalid, mismatched, missing,
  direct-payload-read, and absent metadata behavior.
- Live AArch64 code does not currently read `invariants` or
  `completed_phases` in `validate_prepared_module_handoff(...)`; docs identify
  them as intended handoff gates. Treat that as a blocked status/fail-closed
  row, not authority to demote or ignore the fields.
- `PreparedBirModule::notes` and `BirLoweringResult::notes` are separate
  payloads. Backend LIR failure-message note readers are fallback observers,
  but they are not direct reads of prepared-module notes.
- Step 2 only maps printer/status/debug preservation. It does not prove the
  Step 3 invalid, mismatched, missing, direct-payload-read, or absent metadata
  rows.

## Proof

Analysis-only packet. Required proof:

- `cat todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required because no implementation files were changed.
