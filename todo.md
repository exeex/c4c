Status: Active
Source Idea Path: ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Map Fail-Closed Rows

# Current Packet

## Just Finished

Step 3, `Map Fail-Closed Rows`, completed the analysis-only fail-closed row
map for `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

Fail-closed row statuses:

- `blocked`: current live behavior does not reject the bad metadata state, or
  no field-specific proof exists that rejection would happen.
- `observable-unproven`: current code directly exposes or carries the
  metadata, but the evidence is visibility/preservation only, not a
  fail-closed proof.
- `not-authorized`: the row cannot be satisfied in this packet because the
  required proof would need implementation/test work, expectation changes, or
  metadata authority movement outside Step 3.

Field-by-field fail-closed map:

| Field | Failure row | Expected fail-closed behavior | Current evidence | Proof status and blockers | Compatibility surfaces that must remain stable |
| --- | --- | --- | --- | --- | --- |
| `invariants` | Invalid metadata | Unknown, duplicate-if-disallowed, or semantically impossible invariant evidence must be rejected before a target backend treats the module as prepared. Invalid invariant state must not be accepted because the printer can spell it or because one fixture contains valid enum values. | `PreparedBirInvariant` is a closed enum with `prepared_bir_invariant_name(...)` returning `"unknown"` only after the switch (`src/backend/prealloc/control_flow.hpp:3259-3274`). The prepared printer directly renders every vector entry (`src/backend/prealloc/prepared_printer.cpp:52-57`). No live validator found that checks invariant validity beyond type construction. | `blocked`: invalid enum/payload behavior is not live-proved. Proving this needs a semantic validator/test row that rejects invalid invariant state without relying on undefined enum casts or one named fixture. | Prepared printer invariant block, `--dump-prepared-bir`, focused prepared dumps, AArch64 handoff status, x86 route debug aggregate entry, helper/oracle tests that directly inspect `module.invariants`. |
| `invariants` | Mismatched metadata | If `NoPhiNodes` or `NoTargetFacingI1` is present but the BIR body still contains phi nodes or target-facing i1 payloads, target lowering/status must fail closed instead of trusting the metadata label. If the body is clean but required invariant evidence is absent, it must not be treated as equivalent. | Producers append `NoTargetFacingI1` after legalize and `NoPhiNodes` after out-of-SSA (`src/backend/prealloc/legalize.cpp:456-464`, `src/backend/prealloc/out_of_ssa.cpp:1557-1560`). Tests check expected presence/absence on prepared outputs (`tests/backend/bir/backend_prepare_phi_materialize_test.cpp:207-241`). AArch64 handoff validation checks only target arch/ABI (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: production/presence checks do not prove mismatch rejection. No current status/debug/backend row cross-checks invariant claims against BIR content before lowering. | Prepared invariant printer rows, phi materialization invariant tests, AArch64 prepared handoff, x86 route debug, exact target-output and unsupported behavior. |
| `invariants` | Missing metadata | A module that still requires invariant proof but lacks one or both required invariant entries must fail closed before target lowering or status success. Missing invariant metadata must not silently degrade to “not printed.” | Empty `invariants` simply omits the printer section (`src/backend/prealloc/prepared_printer.cpp:52-57`). AArch64 docs identify invariants as handoff gate carriers, but live handoff code ignores the field (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-58`, `src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: missing invariant metadata currently has no live rejection proof. Printer omission is preservation behavior, not fail-closed behavior. | Printer omission semantics, prepared dump output, AArch64 handoff diagnostics, x86 route debug, existing prepared tests that intentionally inspect invariant presence. |
| `invariants` | Direct payload read | Direct readers of `module.invariants` must either be compatibility mirrors with preserved output or must fail closed when the payload is invalid, mismatched, missing, or absent. A direct read must not become hidden authority for private-pass-context work. | Direct reads found in the prepared printer and tests (`src/backend/prealloc/prepared_printer.cpp:52-57`, `tests/backend/bir/backend_prepare_phi_materialize_test.cpp:36-38`). Step 2 also found dump wrappers reprint the aggregate. No direct backend status reader consumes invariants. | `observable-unproven`: direct payload reads are real and public, but only visibility/preservation is proven. Fail-closed semantics for direct reads remain blocked until a validator defines rejection behavior. | Public `PreparedBirModule::invariants` vector, printer text, dump wrappers, direct test helpers, public prepared aggregate passed to target debug/status routes. |
| `invariants` | Absent metadata | If invariant metadata is absent from an otherwise target-bound prepared module, the backend must reject or explicitly report the missing gate; it must not infer success from route, target profile, or body text alone. | `PreparedBirModule` always has a public vector field, so “absent” is represented as an empty vector or a path that never populated it (`src/backend/prealloc/module.hpp:33-64`). Live AArch64 handoff accepts based on target arch/ABI only (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: absent invariant metadata is not fail-closed today. Proving it requires a field-specific negative row, not docs-only assertions. | AArch64 handoff success/failure contract, prepared dump omission, x86 route debug aggregate behavior, target-output baseline behavior. |
| `completed_phases` | Invalid metadata | Unknown phase names, duplicate-if-disallowed phases, or phase strings outside the configured prepare pipeline must fail closed before being treated as evidence that preparation ran. Invalid phase strings must not be accepted because they print cleanly. | `completed_phases` is a public `std::vector<std::string>` (`src/backend/prealloc/module.hpp:63`). Producers append literal phase strings (`legalize`, `stack_layout`, `liveness`, `out_of_ssa`, `regalloc`). The printer renders every string in order (`src/backend/prealloc/prepared_printer.cpp:44-50`). | `blocked`: there is no live allowlist/order/duplicate validation for phase strings. Printer coverage proves exact rendering of valid fixture strings only. | Prepared printer `completed_phases:` line, CLI prepared dump, focused prepared dump, AArch64 handoff, x86 route debug aggregate. |
| `completed_phases` | Mismatched metadata | Phase metadata that claims a phase ran while the corresponding prepared facts are missing/stale, or a body/fact set that requires a phase not named in the vector, must fail closed before target lowering. | Producers append phase strings at pass completion sites (`src/backend/prealloc/legalize.cpp:456`, `src/backend/prealloc/stack_layout/coordinator.cpp:1674`, `src/backend/prealloc/liveness.cpp:907`, `src/backend/prealloc/out_of_ssa.cpp:1557`, `src/backend/prealloc/regalloc.cpp:471`). AArch64 handoff ignores the vector (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: production order is observable, but no row cross-checks completed phases against required prepared facts or rejects mismatches. | Phase printer order/spelling, prepared dump output, AArch64 handoff diagnostics, target-output behavior, unsupported behavior. |
| `completed_phases` | Missing metadata | Required prepare phases missing from the vector must reject target handoff/status success. Missing phase metadata must not be silently equivalent to an omitted printer line. | Empty `completed_phases` omits the printer line (`src/backend/prealloc/prepared_printer.cpp:44-50`). AArch64 docs identify completed phases as handoff gate carriers, but live validation checks only arch/ABI (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-58`, `src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: no field-specific missing-phase negative proof exists. Docs-only gate language is not proof. | Printer omission semantics, CLI/focused dump, AArch64 handoff, x86 route debug, exact target-output baselines. |
| `completed_phases` | Direct payload read | Direct readers of `module.completed_phases` must preserve public compatibility output or reject bad phase evidence before acting on it. Direct phase reads must not be hidden behind route/status labels or private pass context. | Direct live read found in the prepared printer (`src/backend/prealloc/prepared_printer.cpp:44-50`) and printer fixture (`tests/backend/bir/backend_prepared_printer_test.cpp:2729-2732`, `:2852-2858`). Dump wrappers carry the aggregate through `prepare::print(...)`. | `observable-unproven`: direct payload read preservation is proven for valid text, but no direct reader fails closed for invalid/missing/mismatched phases. | Public `PreparedBirModule::completed_phases`, prepared printer, prepared dump wrappers, status/debug routes receiving the aggregate. |
| `completed_phases` | Absent metadata | A target-bound prepared module with no completed phase evidence must fail closed or explicitly report the missing gate; it must not infer completion from target profile, route name, or downstream facts alone. | The field is always present as a public vector, so absence is an empty vector or unpopulated path (`src/backend/prealloc/module.hpp:63`). Live AArch64 handoff accepts target/ABI-matching modules without checking phases (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | `blocked`: absent phase metadata does not fail closed today. A later proof must reject absence without weakening printer/dump compatibility. | AArch64 handoff, prepared printer/dump omission, route debug, target-output and unsupported behavior. |
| `notes` | Invalid metadata | Malformed prepared notes, empty or invalid phase tags where a note is required, or note payloads that would misrepresent preparation diagnostics must fail closed on status/debug paths that rely on notes. Invalid notes must not be treated as successful metadata just because they render as text. | `PrepareNote` is two public strings (`src/backend/prealloc/names.hpp:21-24`), and the printer renders each note directly (`src/backend/prealloc/prepared_printer.cpp:59-64`). Focus filters erase selected note rows by phase/message text (`src/backend/backend.cpp:588-613`, `:670-686`). | `blocked`: no note schema/validity validator or negative proof was found. Printer/focus behavior is textual compatibility, not validation. | Prepared printer `notes:` block, focused prepared dumps, lowering-note failure messages staying distinct from prepared notes, x86 route debug aggregate. |
| `notes` | Mismatched metadata | Notes whose phase/message claims facts not present in the prepared aggregate, or notes copied from the separate lowering-note channel, must not be accepted as prepared metadata proof. Status/failure text must keep prepared notes distinct from `BirLoweringResult::notes`. | Prepared notes are public aggregate metadata (`src/backend/prealloc/module.hpp:64`); lowering notes are read separately in backend failure-message code (`src/backend/backend.cpp:105-145`, `src/backend/mir/x86/api/api.cpp:11-22`). Step 2 found no live status gate consuming prepared notes. | `blocked`: no live mismatch rejection exists for prepared notes, and separate lowering-note readers cannot prove prepared-note behavior. | Prepared-note printer text, focused note filtering, lowering failure/status message text, x86 route debug/status labels. |
| `notes` | Missing metadata | If a path requires a prepared diagnostic note to explain missing facts, the absence of that note must fail closed or report a structured missing-gate error. Missing notes must not be replaced by generic status labels or lowering notes. | Producers append notes in multiple prepare passes (`src/backend/prealloc/prealloc.cpp:23`, `src/backend/prealloc/legalize.cpp:464`, `src/backend/prealloc/out_of_ssa.cpp:1560`, `src/backend/prealloc/liveness.cpp:1004`, `src/backend/prealloc/regalloc.cpp:813`, `src/backend/prealloc/stack_layout/coordinator.cpp:1723-1739`). The printer omits `notes:` when empty (`src/backend/prealloc/prepared_printer.cpp:59-64`). | `blocked`: no row proves that required notes are required or that missing prepared notes reject handoff/status. Omission is currently printable compatibility only. | Prepared printer omission semantics, focused dump note filtering, lowering-note/status distinction, debug/status outputs. |
| `notes` | Direct payload read | Direct readers of `module.notes` must be compatibility mirrors or must reject invalid/missing/mismatched prepared-note state. Direct reads must not conflate prepared notes with lowering notes or become hidden migration authority. | Direct reads found in the prepared printer and focused prepared dump filters (`src/backend/prealloc/prepared_printer.cpp:59-64`, `src/backend/backend.cpp:588-613`, `:670-686`). Backend failure messages read `BirLoweringResult::notes`, not prepared notes. | `observable-unproven`: direct note reads preserve/render/filter text, but there is no fail-closed validator for the prepared-note payload. | Public `PreparedBirModule::notes`, printer rows, focused dump filtering, separate lowering-note failure messages, x86 route debug aggregate. |
| `notes` | Absent metadata | A prepared module with absent note metadata must not be treated as having satisfied diagnostic/status evidence where notes are required. If notes are optional for a route, the gate must explicitly prove optionality and preserve printer omission. | The field is always present as a public vector, so absence is empty/unpopulated (`src/backend/prealloc/module.hpp:64`). The printer omits notes when empty, and live status gates do not consume prepared notes. | `blocked`: absence behavior is not fail-closed-proven. Optionality is not established by omission or by separate lowering-note readers. | Prepared printer/dump omission, focused dump behavior, lowering-note/status distinction, route debug/status labels. |

Blockers carried forward:

- All invalid, mismatched, missing, and absent rows are blocked for
  fail-closed proof. Current live code exposes or preserves the metadata but
  does not reject bad, missing, mismatched, or absent metadata for these three
  fields.
- Direct payload reads are observable for printer/tests/focused dumps, but
  direct-read visibility is not fail-closed proof and does not authorize
  metadata demotion, wrapper migration, or private-pass-context work.
- `PreparedBirModule::notes` remains distinct from `BirLoweringResult::notes`;
  lowering failure-note behavior cannot satisfy prepared-note rows.
- No row is satisfied by one named fixture, weaker expectations, docs-only
  assertions, classification-only notes, or status/debug relabeling.
- No demotion, privatization, wrapper, migration, private-pass-context, or
  implementation work is authorized by this Step 3 map.

## Suggested Next

Execute Step 4 from `plan.md`: decide the gate status from the Step 2
preservation matrix and this Step 3 fail-closed map. The likely gate decision
is blocked unless the supervisor first authorizes a separate implementation or
test-proof lifecycle idea for the blocked rows.

## Watchouts

- This active plan is proof-gate work only.
- Do not demote, delete, privatize, wrap, migrate, or implement authority
  movement for `PreparedBirModule` metadata.
- Do not weaken printer, status, debug, helper/oracle, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  status/oracle relabeling, printer/debug formatting changes,
  classification-only notes, docs-only assertions, or named-fixture proof.
- AArch64 handoff currently validates target arch and ABI only; it does not
  validate `invariants` or `completed_phases`.
- x86 route debug receives the prepared aggregate but does not directly render
  or validate these three fields.
- Focused prepared dump note filtering is textual compatibility behavior, not
  proof that prepared notes fail closed.
- Step 3 maps fail-closed rows only. It does not authorize Step 4 gate closure
  by itself.

## Proof

Analysis-only packet. Required proof:

- `cat todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required because no implementation files were changed.
