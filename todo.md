# Current Packet

Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add focused backend probes for unpublished load-local ALU operands

## Just Finished

Step 2 added two focused backend probes under `tests/backend/case/` for the
remaining unpublished `load_local` / stale-home ALU operand seam:

- `tests/backend/case/aarch64_alu_unpublished_load_local_after_call.c`
  isolates a post-`printf` materialization event followed by ordinary
  `load_local -> and -> xor -> or` scalar ALU consumers. It distinguishes ALU
  operand selection after a variadic call/materialization from the larger
  external testcase's surrounding logical/select chain.
- `tests/backend/case/aarch64_alu_unpublished_load_local_call_boundary.c`
  varies the same-feature shape by inserting a direct noinline call boundary
  before the same `load_local -> and -> xor -> or` scalar ALU chain. It
  distinguishes a general call-boundary clobber/source-producer gap from a
  printf-specific variadic call argument route.

Both probes are registered as AArch64 route-only backend tests in
`tests/backend/CMakeLists.txt`. They do not require AArch64 runtime emulation
and passed on current HEAD without weakening behavior expectations.

## Suggested Next

Run Step 3 against the focused probes by routing scalar load source lookup
through prepared memory/source authority, then prove the two registered probes
plus the existing four-test baseline.

## Watchouts

Fresh Step 1 baseline pass/fail split remains:
`c_testsuite_aarch64_backend_src_00164_c` failed,
`c_testsuite_aarch64_backend_src_00176_c` passed,
`c_testsuite_aarch64_backend_src_00181_c` passed, and
`c_testsuite_aarch64_backend_src_00204_c` failed.

The focused probe dumps both show the stale-home candidate shape as ordinary
ALU operands loaded after a call/materialization boundary. That keeps the
remaining seam classified as ALU-owned or shared source-producer-owned for the
next packet, not calls-owned yet: the probe ALU chain is not a call-argument
materialization path.

The `00204` failure still starts in the stdarg output after otherwise-correct
argument and return-value sections; keep it as a secondary signal until the
smaller `00164` ALU chain is reduced.

Do not widen directly into `calls.cpp` under this plan. If the probes prove the
remaining stale-home decision is owned by call argument or call boundary
materialization, stop and route through
`ideas/open/52_aarch64_calls_prepared_authority_repair.md`.

Do not resume `dispatch_value_materialization.cpp` close-readiness work until
the ALU-owned stale-home baseline is reduced or reclassified.

## Proof

Focused registered-probe proof ran and is preserved in `test_after.log`:

`cmake --build --preset default`

`ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)$'`

Then, for route-shape observation:

`build/c4cll --dump-prepared-bir --target aarch64-linux-gnu <probe> --mir-focus-function main`

Result: build succeeded after CMake regeneration; both registered probe tests
passed, and the prepared-BIR dumps show the intended `load_local` ALU operand
chain.
