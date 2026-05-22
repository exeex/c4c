Status: Active
Source Idea Path: ideas/open/356_semantic_bir_pointer_derived_string_loads.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh Current First Bad Fact refreshed the current `00173`
semantic pointer-derived string-load proof. The focused subset passed 5/5 with
0 failures, including `c_testsuite_aarch64_backend_src_00173_c`; no guardrail
exposed dynamic pointer-derived byte-load collapse to fixed global-byte loads
as a current in-scope first bad fact.

## Suggested Next

Ask the plan owner for the close/park decision using the refreshed green proof
and the required close-gate policy.

## Watchouts

- The source idea is parked and says the dynamic pointer-derived byte-load
  owner was previously repaired; this refresh did not produce fresh evidence to
  reopen implementation work.
- Do not reopen string literal pointer-value publication, AArch64
  address-valued publication, recursive formal preservation, or frontend
  admission work inside this plan without lifecycle routing.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|c_testsuite_aarch64_backend_src_00173_c)$' > test_after.log 2>&1
```

Result: build was up to date; focused subset passed 5/5 with 0 failures.
`test_after.log` is the proof log path. The supervisor-selected proof was
sufficient for this refresh/classification packet.
