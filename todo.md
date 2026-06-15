Status: Active
Source Idea Path: ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Metadata Authority

# Current Packet

## Just Finished

Step 1, `Inventory Metadata Authority`, completed the analysis-only inventory
for `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

Definitions and construction:

- `PreparedBirModule` publicly stores `invariants`, `completed_phases`, and
  `notes` in `src/backend/prealloc/module.hpp:38`, `:41`, and `:63-64`.
  `PreparedBirInvariant` is the two-value enum
  `NoTargetFacingI1`/`NoPhiNodes`, with printer names in
  `src/backend/prealloc/control_flow.hpp:3260-3273`; `PrepareNote` is the
  public `{phase, message}` payload in `src/backend/prealloc/names.hpp:26-29`.
- `BirPreAlloc(const bir::Module&, TargetProfile, PrepareOptions)` creates an
  empty `PreparedBirModule` and stores only the source BIR plus target profile
  before phases populate metadata (`src/backend/prealloc/module.hpp:379-386`).
  The alternate `BirPreAlloc(PreparedBirModule, PrepareOptions)` accepts an
  existing public aggregate and resolves a missing target profile from the
  module triple (`src/backend/prealloc/module.hpp:387-395`), so all three
  fields are direct payload inputs for compatibility fixtures and wrappers.
- `BirPreAlloc::run()` publishes the completed aggregate by value after
  running `legalize`, `stack_layout`, `liveness`, `out_of_ssa`, `regalloc`,
  and `publish_contract_plans` (`src/backend/prealloc/prealloc.cpp:29-37`).
  Public creation paths are `prepare_semantic_bir_module_with_options(...)`
  and `prepare_bir_module_with_options(...)`
  (`src/backend/prealloc/prealloc.cpp:59-70`), plus
  `prepare_semantic_bir_pipeline(...)` in `src/backend/backend.cpp:165-169`.

Field inventory:

| Field | Producers | Readers and observers | Retained public prepared compatibility surfaces |
| --- | --- | --- | --- |
| `invariants` | `run_legalize()` appends `NoTargetFacingI1` when target-facing `i1` promotion is active (`src/backend/prealloc/legalize.cpp:455-462`). `run_out_of_ssa()` appends `NoPhiNodes` after out-of-SSA rewrites (`src/backend/prealloc/out_of_ssa.cpp:1556-1560`). Tests may construct/directly mutate fixtures, e.g. printer fixture at `tests/backend/bir/backend_prepared_printer_test.cpp:2739-2743`. | `prepare::print(...)` directly reads the vector and prints `invariants:` rows with `prepared_bir_invariant_name(...)` (`src/backend/prealloc/prepared_printer.cpp:52-57`). `backend_prepare_phi_materialize_test` directly scans `module.invariants` as a payload read (`tests/backend/bir/backend_prepare_phi_materialize_test.cpp:37-38`) and checks both invariant names/absence rows around `:208-241`. `dump_generic_prepared_bir(...)` preserves or filters the public aggregate and then calls `prepare::print(...)` (`src/backend/backend.cpp:287-343`). CLI dump expectations observe the prepared dump path (`tests/backend/bir/CMakeLists.txt:550-559`). AArch64 docs name invariants as handoff gates (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-55`, `BIR_PREPARED_GAP_LEDGER.md:134` and `:159`), but live `validate_prepared_module_handoff(...)` currently checks target arch/ABI only (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | Public struct field in `module.hpp`; public aggregate construction through `BirPreAlloc` and `prepare_*_bir_module_with_options`; public prepared dump through `prepare::print(...)` and `--dump-prepared-bir`; target aggregate handoffs through x86 `emit_prepared_module(...)`, AArch64 `compile_prepared_module(...)`/`build_module(...)`, and RISC-V `emit_prepared_module(...)`. |
| `completed_phases` | Each phase appends its stable phase name: `legalize` (`src/backend/prealloc/legalize.cpp:455-456`), `stack_layout` (`src/backend/prealloc/stack_layout/coordinator.cpp:1673-1675`), `liveness` (`src/backend/prealloc/liveness.cpp:906-908`), `out_of_ssa` (`src/backend/prealloc/out_of_ssa.cpp:1556-1558`), and `regalloc` (`src/backend/prealloc/regalloc.cpp:470-471`). Printer tests directly set fixture phases at `tests/backend/bir/backend_prepared_printer_test.cpp:2729-2732`. | `prepare::print(...)` directly reads and prints `completed_phases:` in vector order (`src/backend/prealloc/prepared_printer.cpp:44-50`). Backend prepared dumps call that printer directly or after focus filtering (`src/backend/backend.cpp:302-343`, `:1500-1529`). CLI dump test requires the exact phase row for stdarg (`tests/backend/bir/CMakeLists.txt:550-559`). AArch64 docs identify completed phases as required gate carriers (`src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md:54-55`, `BIR_PREPARED_GAP_LEDGER.md:134` and `:159`), while live AArch64 handoff validation does not read the field yet (`src/backend/mir/aarch64/abi/abi.cpp:870-881`). | Same public aggregate surfaces as `invariants`: direct public field, prepare API return, prepared printer/dump, backend dump focus wrappers, and target prepared-module handoffs. |
| `notes` | `BirPreAlloc::note(...)` appends the prealloc route note before phase execution (`src/backend/prealloc/prealloc.cpp:22-31`). Phase producers append notes in `run_legalize()` (`src/backend/prealloc/legalize.cpp:464-470`), stack-layout addressing/fallback helpers via `prepared_.notes` (`src/backend/prealloc/stack_layout/coordinator.cpp:1705-1714`) plus per-function and summary notes (`:1723-1744`), `run_liveness()` (`src/backend/prealloc/liveness.cpp:1004-1010`), `run_out_of_ssa()` (`src/backend/prealloc/out_of_ssa.cpp:1560-1565`), and `run_regalloc()` (`src/backend/prealloc/regalloc.cpp:813-824`). Printer tests also push direct fixture notes at `tests/backend/bir/backend_prepared_printer_test.cpp:2729-2735`. | `prepare::print(...)` directly reads notes and renders `notes:` rows (`src/backend/prealloc/prepared_printer.cpp:59-64`). Backend focused prepared dumps copy the aggregate and erase selected note rows when filtering function/block/value focus (`src/backend/backend.cpp:588-613`, `:670-686`) before printing. Prepared dump tests require note text through CLI snippets (`tests/backend/bir/CMakeLists.txt:550-559`), and stack-layout tests directly iterate `prepared.notes` (`tests/backend/bir/backend_prepare_stack_layout_test.cpp:5158`). LIR lowering notes are separate `BirLoweringResult::notes` payloads used by backend failure messages (`src/backend/backend.cpp:105-145`, `src/backend/mir/x86/api/api.cpp:11-22`) and should not be conflated with `PreparedBirModule::notes`. | Public struct field and direct fixture mutation; prepared printer/dump text; backend focused dump filters over retained prepared notes; x86/AArch64/RISC-V prepared aggregate handoffs, where notes remain carried with the aggregate even when target-specific emitters do not directly inspect this field. |

Observed path map:

- Printer/status/debug: `prepare::print(...)` is the only field-specific
  printer for all three metadata fields. x86 route summary/trace wrappers
  (`src/backend/mir/x86/debug/debug.cpp:511-524` and
  `src/backend/mir/x86/codegen/route_debug.hpp:7-23`) observe the aggregate
  for route diagnostics, but do not directly print these metadata fields.
  `dump_module(...)` dispatches `PreparedBir` to `prepare::print(...)` and
  `MirSummary`/`MirTrace` to x86 debug (`src/backend/backend.cpp:1493-1529`).
- Helper/oracle/fallback/wrapper: x86 public API prepares BIR and forwards the
  aggregate to `emit_prepared_module(...)` (`src/backend/mir/x86/api/api.cpp:27-58`);
  x86 module emit consumes the public aggregate after target-profile validation
  (`src/backend/mir/x86/module/module.cpp:6452-6459`). AArch64 build keeps a
  pointer to the original prepared aggregate in the target module shell
  (`src/backend/mir/aarch64/codegen/module_compile.cpp:18-28`, `:73-83`), and
  handoff tests assert that identity is retained
  (`tests/backend/mir/backend_aarch64_prepared_handoff_gate_test.cpp:33-50`).
  RISC-V exposes `emit_prepared_module(const PreparedBirModule&)`
  (`src/backend/mir/riscv/codegen/emit.hpp:95-96`).
- Target-output and unsupported paths: public backend entry points prepare the
  aggregate before x86, AArch64, and RISC-V target emission
  (`src/backend/backend.cpp:1333-1352`, `:1371-1394`). Unsupported or generic
  fallback paths may prepare the aggregate but render only `prepared.module`
  BIR text through `render_prepared_bir_text(...)`, so the metadata is carried
  but not printed there (`src/backend/backend.cpp:159-168`, `:1367-1368`,
  `:1409-1412`, `:1487-1490`).
  AArch64 assembly output validates target arch/ABI and lowers from the
  aggregate (`src/backend/mir/aarch64/codegen/asm_emitter.cpp:242-255`) but
  does not yet enforce `invariants` or `completed_phases` despite docs marking
  them as intended gates.

## Suggested Next

Execute Step 2 from `plan.md`: turn the Step 1 inventory into a
field-by-field preservation matrix for printer, status, and debug behavior.
Mark rows as already proved, directly observable but unproven, or blocked.

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
  them as intended handoff gates. Treat that as an unproven preservation/fail-
  closed row, not authority to demote or ignore the fields.
- `PreparedBirModule::notes` and `BirLoweringResult::notes` are separate
  payloads. Backend LIR failure-message note readers are fallback observers,
  but they are not direct reads of prepared-module notes.

## Proof

Analysis-only packet. Required proof:

- `cat todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required because no implementation files were changed.
