# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Raw Label Fallbacks And Proof Baseline

## Just Finished

`plan.md` Step 1 inventoried raw BIR label fallback dependencies across the
target surfaces and recorded the first executable cleanup packet.

Fallback classification:

- `src/backend/bir/bir.hpp`: `bir::Block::label`,
  `bir::BranchTerminator::target_label`,
  `bir::CondBranchTerminator::{true_label,false_label}`,
  `bir::Terminator::{target_label,true_label,false_label}`,
  `bir::PhiIncoming::label`, and `bir::MemoryAddress::base_name` for
  `BaseKind::Label` are raw spelling payloads paired with optional
  `BlockLabelId` fields. Classification: construction convenience plus
  rendering compatibility; when the id remains `kInvalidBlockLabel`, the same
  raw string becomes an unresolved-id fallback.
- `src/backend/bir/lir_to_bir/module.cpp`: `intern_known_block_labels` interns
  block labels, then fills phi incoming and branch target ids by matching raw
  labels against a per-function `known_labels` map. Classification: acceptable
  construction convenience for LIR-to-BIR handoff, but unknown labels still
  leave unresolved-id fallback in BIR.
- `src/backend/bir/bir_printer.cpp`: `render_block_label` prefers
  `BlockLabelId` table spelling and falls back to raw spelling for block labels,
  branch targets, cond-branch targets, phi incoming labels, and label-address
  bases. Classification: rendering compatibility fallback.
- `src/backend/prealloc/legalize.cpp`: `intern_prepared_block_label` prefers
  BIR ids and falls back to raw labels while building prepared branch
  conditions and prepared control-flow blocks. Classification: explicit
  unresolved-id fallback at the BIR-to-prepared boundary.
- `src/backend/prealloc/prealloc.hpp`: prepared model fields carry
  `BlockLabelId`; helper overloads such as `resolve_prepared_block_label_id`,
  string-view `find_prepared_linear_join_predecessor`, and string-view
  `find_prepared_parallel_copy_bundle` resolve raw labels through prepared name
  tables. Classification: mixed compatibility boundary; id overloads are the
  preferred authority.
- `src/backend/prealloc/liveness.cpp`: block index construction interns
  `block.label`, successors resolve `terminator.*_label`, and phi uses resolve
  `incoming.label`. Classification: legacy downstream authority over raw BIR
  spelling.
- `src/backend/prealloc/out_of_ssa.cpp`: `find_block`, reachability,
  `BlockAnalysis::blocks_by_label`, phi join classification, and
  `make_join_transfer` still compare or intern `bir::Block::label`,
  `terminator.*_label`, and `PhiIncoming::label`. Classification: legacy
  downstream authority with some nearby id-first prepared helpers already
  present.
- `src/backend/prealloc/stack_layout/coordinator.cpp`: addressing access
  collection interns each `block.label` for `PreparedAddressingAccess`.
  Classification: legacy downstream authority over raw BIR block spelling.
- `src/backend/prealloc/stack_layout/alloca_coalescing.cpp`: phi incoming block
  lookup uses a raw `block.label` index and `incoming.label`. Classification:
  legacy downstream authority for local-slot coalescing.
- `src/backend/prealloc/prealloc.cpp`: dynamic stack plan uses
  `prepared.names.block_labels.find(block.label)`. Classification: legacy
  downstream authority, but only after prepared name tables have been built.
- `src/backend/prealloc/prepared_printer.cpp`: prepared labels render from ids
  and print `<none>` for invalid ids; join-transfer retained `incoming.label`
  text is dump compatibility. Classification: mostly id-authoritative
  rendering, with phi incoming raw spelling retained for compatibility output.
- `src/backend/backend.cpp`: focus-block filtering accepts raw selector text
  from CLI, resolves it through prepared name tables, and then filters prepared
  metadata by ids. Classification: selector spelling boundary, not a cleanup
  target unless focus behavior changes.
- Backend tests under `tests/backend/` construct many raw-only BIR fixtures
  with `.label`, `.target_label`, `.true_label`, `.false_label`, and
  `PhiIncoming{.label=...}`; structured-context tests also cover
  `base_label_id`. Classification: test construction convenience plus retained
  compatibility coverage; future cleanup should convert ordinary supported
  fixtures to attach ids and leave raw-only cases explicit.

## Suggested Next

First cleanup packet: convert ordinary BIR construction fixtures and validation
helpers for blocks, branch targets, conditional branch targets, phi incoming
labels, and label-address bases to attach `BlockLabelId` when a `NameTables`
instance is available. Keep raw spelling as display text only; retain raw-only
fixtures only where the test explicitly covers compatibility or invalid-id
behavior.

Owned files for that packet should include `src/backend/bir/bir.hpp` only if a
small helper belongs with the data model, plus the focused backend test files
whose fixtures are converted. Start with structured-context and prepared
coverage before touching prepared consumers:
`tests/backend/backend_prepare_structured_context_test.cpp`,
`tests/backend/backend_prepare_liveness_test.cpp`,
`tests/backend/backend_prepare_block_only_control_flow_test.cpp`, and any
existing shared backend test fixture helper found during implementation.

Expected fallback classification after that packet: ordinary construction
becomes id-backed construction convenience plus rendering compatibility; any
remaining raw-only fixture must be named as invalid-id compatibility, not
silent downstream authority.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not expand into MIR migration unless the supervisor opens a separate
  initiative.
- Avoid testcase-overfit cleanup that only makes one known case pass.
- `c4c-clang-tool-ccdb` is installed, but `build/compile_commands.json` did not
  contain the queried backend sources, so this inventory used targeted `rg` and
  source snippets for those files.
- Do not start by rewriting prepared liveness/out-of-SSA lookups. They still
  depend on raw BIR spelling because Step 2 has not yet eliminated raw-only BIR
  construction for ordinary supported paths.

## Proof

Inventory-only packet. No build or test command was run, and no `test_after.log`
was produced.

Recommended proof for the first cleanup packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
