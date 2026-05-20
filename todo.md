Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the target long-double literal/data representation owner for
AArch64 by preserving raw float literal spellings in HIR and using those
spellings to format non-x86 `long double` constants as LLVM `fp128` binary128
payloads. The shared `fp128` formatter now emits high bits before low bits in
LLVM `0xL...` spelling, so BIR global initializer byte lowering receives a
true binary128 payload instead of a double-rounded or x87-shaped payload.

Generated `build/c_testsuite_aarch64_backend/src/00204.c.s` now encodes
`hfa31 = { 31.1L }` as the Clang-matching little-endian binary128 bytes:
`9a 99 99 99 99 99 99 99 99 99 99 99 99 f1 03 40`. The existing `.str44`
publication remains intact with `adrp/add x0, .str44`, `mov v0.16b, ...`, and
`mov v1.16b, ...` before `bl printf`.

Direct related source outside the original owned code list was required:
`src/frontend/hir/hir_ir.hpp` and `src/frontend/hir/impl/expr/expr.cpp` own the
HIR float literal payload and were the only place the raw parser spelling could
be carried into constant lowering.

## Suggested Next

Continue Step 2 with the remaining HFA multi-lane argument/return publication
failure. After the binary128 data repair, the first visible `00204.c` mismatch
is no longer the `31.1L` global bytes; the run now prints `13.1 0.0 -nan`
where the expected HFA argument line is `13.1 13.2 13.3`, and later segfaults
after corrupted return-value output.

## Watchouts

The new decimal-to-binary128 formatter is intentionally bounded to literals
that fit through the local `unsigned __int128` path and falls back to the
existing double-based conversion for oversized or unsupported spellings. That
keeps this packet on literal/data representation without expanding into a full
arbitrary-precision frontend initiative.

Do not reopen the already committed HFA lane-home, F128 transport, memory
retargeting, or call ABI register-indexing surfaces for this payload repair.
The remaining failure is not the `31.1L` byte sequence and should be proved from
fresh generated-code evidence rather than expectation rewrites.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`
passed. `c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_NONZERO` / segmentation fault. The repaired `hfa31` bytes and `.str44`
q0/q1 publication were confirmed in generated `00204.c.s`; the fresh remaining
first bad fact is HFA multi-lane argument output, starting with `13.1 0.0 -nan`
instead of `13.1 13.2 13.3`. Also ran `git diff --check`, which passed.
`test_after.log` is the fresh proof log for this dirty state.
