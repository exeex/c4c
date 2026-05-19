Status: Active
Source Idea Path: ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement PIC-Safe External Data-Symbol Address Formation

# Current Packet

## Just Finished

Step 2 implemented PIC-safe external data-symbol address formation across the
semantic and remaining AArch64 producer paths owned by this packet.

`src/backend/bir/lir_to_bir/globals.cpp` now lowers AArch64
`LirGlobal::is_extern_decl` data globals to
`bir::GlobalAddressMaterializationPolicy::GotRequired` before the static
relocation-model fallback, while internal and same-module compiler-owned data
globals keep `Direct`.

`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` now has a semantic
regression proving an AArch64 extern data global gets `GotRequired` and both an
internal global and a same-module owned global stay `Direct`.

`src/backend/mir/aarch64/codegen/dispatch.cpp` now recognizes
`bir::LoadGlobalInst` targets whose BIR global policy is `GotRequired` and
emits `adrp <reg>, :got:<symbol>` / `ldr <reg>, [<reg>, :got_lo12:<symbol>]`
before loading the data value. The same policy check is also used by the
recursive call-argument value materializer so it no longer emits direct
`adrp`/`:lo12:` for externally binding data globals.

`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now covers
both dispatch sides: a GOT-required extern data global load uses GOT
page/low12 addressing, while a direct internal data global load keeps direct
page/low12 addressing.

The old `00189.c` relocation failure is gone. The generated assembly now
contains `adrp x10, :got:stdout` / `ldr x10, [x10, :got_lo12:stdout]` /
`ldr x9, [x10]` for the later `stdout` load, and no longer contains the old
`adrp x10, stdout` / `add x10, x10, :lo12:stdout` sequence.

New residual: `c_testsuite_aarch64_backend_src_00189_c` now links and runs, but
fails at runtime with `Segmentation fault`. The visible route still has
pre-existing call/argument lowering problems around the final indirect
`fprintf` call (`mov x1, x20`, `ldr w2, [sp, #8]`, `blr x21`) that are separate
from PIC-safe external data-symbol address formation.

## Suggested Next

Delegate a separate packet for the now-unmasked `00189.c` runtime failure in
the AArch64 call/argument route. The immediate evidence is that the final
indirect `fprintf` call is not receiving the expected argument/register setup
after the relocation-safe `stdout` load.

## Watchouts

- Do not implement filename-specific or `stdout`-specific shortcuts.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof logs to claim
  progress.
- Preserve legal direct local/internal symbol address formation.
- Do not route string constants, labels, TLS, or same-module/internal function
  symbols through the external-data GOT rule.
- The delegated proof now fails after link, at runtime. Do not conflate the new
  segmentation fault with the fixed relocation issue.
- `backend_aarch64_instruction_dispatch_test` was run directly and passes the
  new GOT-required/direct-global dispatch coverage, but it is not part of the
  supervisor-selected CTest regex.

## Proof

Build proof passed: `cmake --build --preset default`.

Focused local test passed:
`./build/tests/backend/mir/backend_aarch64_instruction_dispatch_test`.

Delegated CTest proof was run exactly and preserved in `test_after.log`:
`ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_stack_layout|backend_aarch64_prepared_memory_operand_records|backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log`.

Result: 4/5 selected tests passed (`backend_lir_to_bir_notes`,
`backend_prepare_stack_layout`, `backend_aarch64_prepared_memory_operand_records`,
and `backend_aarch64_machine_printer`). `c_testsuite_aarch64_backend_src_00189_c`
now fails at runtime with `[RUNTIME_NONZERO] ... exit=Segmentation fault`.
The old `R_AARCH64_ADR_PREL_PG_HI21` relocation against `stdout@@GLIBC_2.17`
is no longer present.

Supervisor acceptance checks:

- Non-decreasing regression guard passed:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
- Broader AArch64 backend unit sanity passed:
  `ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'`
  selected 27 tests and passed 27/27.
- `git diff --check` passed.
