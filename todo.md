Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Source-Route Equivalence And Failure Tests

# Current Packet

## Just Finished

Step 5 adds source-route equivalence coverage for the supported one-function
RV64 object subset.

- `backend_c4c_as_parse_suite` now receives both `c4c-as` and `c4cll`, plus the
  source-level inline asm case `tests/backend/case/riscv64_vrm_insn_d_source.c`.
- The focused suite emits a canonical object through `c4c-as`, emits the same
  source-level inline asm route through
  `c4cll --codegen obj --target riscv64-linux-gnu`, extracts `.text` bytes from
  both ELF objects, and requires both to equal
  `0a0320080b0300001305000067800000`.
- Existing fail-closed coverage for instructions outside `.text` and unsupported
  directives remains in the same suite.

## Suggested Next

Supervisor should review whether the active runbook is complete enough for
lifecycle close/deactivation or whether it needs a follow-up source idea for
broader assembler syntax beyond the current one-function subset.

## Watchouts

- The equivalence test compares `.text` bytes, not whole-object bytes; the
  current object writer can still differ in metadata layout without affecting
  this route contract.
- The current object subset remains intentionally one global function with a
  matching label; broader label layout, multiple functions, relocations, and
  extra sections remain non-goals for this plan.

## Proof

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed. Proof log: `test_after.log`.
