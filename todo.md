Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Object-Emission Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed the focused `backend_riscv_object_emission`
evidence and named the first observable boundary. The focused row reproduces:
the delegated build passed, and the focused CTest failed with details in
`test_after.log`.

Evidence summary:
`build/agent_state/664_step1_object_emission_probe/summary.md`.

Prepared/RV64 status: the first failing fixture
`loads_rv64_va_start_published_word_after_helper()` has valid prepared input
for a post-helper `LoadLocalInst`, register home, and prepared memory access.
RV64 object text emission accepts the prepared module but emits only the stale
56-byte `va_start` helper sequence, omitting the expected post-helper load.
Section, symbol, relocation, and ELF writer output are structurally valid for
that stale text and report no diagnostic.

## Suggested Next

Step 2 should select RV64 prepared instruction traversal/text emission as the
first owner: a successful variadic helper lowering must continue consuming
subsequent prepared instructions before object-module publication. Route to an
executor only after Step 2 records the positive contract and fail-closed
negative shape.

## Watchouts

- Keep this route at the RISC-V object-emission infrastructure layer until
  focused evidence proves a different first owner.
- Do not absorb unrelated RV64 runtime rows into this object-emission route.
- The focused test also reports later relocation, byval, incoming formal,
  local-memory, pointer-value, sret, call-argument ordering, and diagnostic
  exactness failures; classify the first `va_start` post-helper instruction
  consumption boundary before selecting any later row.
- Do not use testcase identity, final object bytes alone, expectation rewrites,
  unsupported-marker changes, allowlists, timeouts, runtime policy, or
  baseline accounting as progress.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest failed. The failure is the expected probe
evidence for this Step 1 packet, and `test_after.log` is the canonical proof
log.
