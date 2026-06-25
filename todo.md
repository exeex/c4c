Status: Active
Source Idea Path: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Broader Validation And Closure Review

# Current Packet

## Just Finished

Step 6 completed broader backend validation and closure review for idea 349.

- Ran the broader backend subset after parser, encoder, object-route rewire,
  and shared text/object compatibility tests.
- Validation passed: `100% tests passed, 0 tests failed out of 319`.
- Final route review found no testcase-overfit signal: object bytes and `.s`
  expectations were preserved, and `.insn.d` object emission now consumes
  substituted canonical line text through the shared parser/encoder.

## Suggested Next

Call the plan owner to close idea 349 if it accepts that the source idea is
complete.

Closure evidence:

- the line core parses and encodes `.insn.d`, `li`, and `ret`
- inline asm object emission substitutes to canonical text before line parsing
- source-level `.insn.d` object bytes remain
  `0a0320080b0300001305000067800000`
- source-level `.s` output remains canonical and parseable
- malformed line/parser cases fail closed in focused tests

## Watchouts

- Do not start ideas 350 or 351 before idea 349 provides the shared line core.
- Do not add new testcase-shaped `.insn.d` object special cases.
- Keep inline asm constraints and VRM allocation out of the line parser.
- Legacy `.insn.d` carrier classifier helpers still exist for tests and
  comparison; do not route new object behavior back through prepared
  placeholder fields.
- `li` and `ret` encoding helpers already exist indirectly as
  `append_rv64_load_immediate(...)` plus the `jalr x0, 0(ra)` return encoding,
  but they are not exposed as a line encoder yet.
- Keep source-level object byte assertion
  `0a0320080b0300001305000067800000` intact.

## Proof

Broader validation passed and was captured in `test_after.log`:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: 319/319 tests passed.
