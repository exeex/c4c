# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Validation And Completion Review

## Just Finished

`plan.md` Step 5 bounded assembler/backend handoff proof was added without
recovering broad x86 scalar/module rendering.

Completed work:

- `src/backend/mir/x86/module/module.cpp`: the contract-first x86 module handoff
  now validates prepared control-flow label authority before stub rendering. It
  requires prepared function/control-flow records, resolves prepared block label
  ids through the prepared name table, maps those ids back to BIR block
  identities, and rejects invalid, missing, or drifted prepared branch labels
  when prepared control-flow metadata is present. Existing prepared-module
  contract fixtures that intentionally publish frame/regalloc/storage authority
  without any prepared control-flow records remain accepted by the x86 stub
  emitter.
- `tests/backend/backend_x86_prepared_handoff_label_authority_test.cpp`: added a
  focused x86 prepared-module handoff test proving that valid prepared label ids
  are consumed even when raw BIR label spelling is drifted, while a missing
  prepared label id and a drifted prepared branch-condition label are rejected
  with the canonical prepared-module handoff message.
- `tests/backend/CMakeLists.txt`: registered
  `backend_x86_prepared_handoff_label_authority_test` as a narrow backend target
  independent of the unavailable broad x86 handoff boundary suite.

## Suggested Next

Execute `plan.md` Step 6: run final backend validation selected by the
supervisor, review remaining raw-label fallback sites against the source idea
acceptance criteria, and record any intentionally retained compatibility
boundaries plus proof results here for closure judgment.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not remove raw label spelling fields yet. They are still the compatibility
  payload for printer output, unresolved-id fallback, and existing downstream
  code not covered by this packet.
- `clang-format` is not installed in this container, so this packet was kept
  manually formatted.
- The hook intentionally validates prepared control-flow metadata before the
  current x86 stub fallback; it does not add handwritten scalar BIR-shape
  rendering or change dump spelling.
- Prepared modules with an empty `control_flow.functions` table are a retained
  compatibility boundary for older contract fixtures. Do not turn that absence
  into a hard x86 handoff failure unless the source fixture starts publishing
  prepared control-flow records.
- Raw label spelling remains retained for BIR/printer compatibility; this proof
  only establishes the x86/backend handoff boundary for prepared ids.

## Proof

Step 5 acceptance proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=OFF && cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_' ) > test_after.log 2>&1`

Proof log: `test_after.log`

Result: passed, 108/108 enabled backend tests; 12 optional x86 MIR trace tests
were disabled by the normal `C4C_ENABLE_X86_BACKEND_TESTS=OFF` configuration.
