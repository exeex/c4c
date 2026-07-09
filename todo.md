Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Residual Row

# Current Packet

## Just Finished

Step 1 refreshed the `src/ipa-sra-2.c` RV64 torture residual from the current
tree with the exact delegated command:

```sh
(cmake --build --preset default && ALLOWLIST=build/agent_state/643_step1_ipa_sra_2.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

Result: build completed, the one-row allowlist probe returned nonzero, and the
row remains failing: `total=1 passed=0 failed=1`.

Artifacts:
- case log: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/case.log`
- summary: `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- failed list: `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- work dir: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c`
- C4C object: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/c4c.o`
- C4C binary: `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/c4c.bin`
- clang control binary:
  `build/rv64_gcc_c_torture_backend/src_ipa-sra-2.c/clang.bin`

Stage boundary:
- C4C object compile completed and produced `c4c.o`.
- ELF sanity completed; the object is ELF64 little-endian RISC-V relocatable
  with machine `RISC-V`.
- clang control build completed and the clang binary exited `0`.
- C4C link completed and produced `c4c.bin`.
- First observable failure is C4C runtime under QEMU: the comparison reports
  `clang_exit=0 c4c_exit=Segmentation fault`, with no stdout/stderr payload
  from either binary.

## Suggested Next

Execute Step 2: trace the scalar freshness boundary for `src/ipa-sra-2.c`,
starting from the generated `c4c.o`/`c4c.bin` and case log, and identify the
prepared value, call instruction, post-call consumed home, clobber facts, and
expected fresh source.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- Do not weaken call-clobber, preservation, republication, rematerialization,
  expectation, unsupported-marker, allowlist, timeout, or pass/fail accounting
  behavior.
- The current refresh proves the row reaches runtime, but Step 1 does not by
  itself prove scalar call-boundary freshness is the first internal owner; Step
  2 must confirm or reassign that owner from object/disassembly evidence.

## Proof

Proof log: `test_after.log`.

Command:

```sh
(cmake --build --preset default && ALLOWLIST=build/agent_state/643_step1_ipa_sra_2.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

The proof is sufficient for this evidence-refresh packet: it rebuilt the tree
and refreshed the one-row residual boundary. The nonzero exit is the expected
residual result for a failing allowlist row, not an executor blocker.
