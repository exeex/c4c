Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 localized and repaired the first `fa4` entry-consumption owner inside
the delegated AArch64 MIR surface. The caller already publishes the fixed mixed
signature coherently as `w0`, `s0..s3`, `w1`, `d4..d7`, `w2`, and stack
`q` lanes for `hfa34`; the callee entry path was consuming register formals by
flat parameter index, so mixed GPR/FPR signatures read later small aggregates
from `w5` and HFA lanes from shifted FP/SIMD registers.

AArch64 entry-formal publication now computes the incoming ABI register index
per register bank before selecting the source register. This fixes the
register-source owner generally for interleaved fixed GPR, scalar FP, and HFA
formals rather than special-casing `00204.c` or `fa4`.

Focused backend coverage now pins the repaired owner. The
`backend_aarch64_instruction_dispatch` bucket includes an interleaved fixed
formal shape matching the ABI pressure pattern: small byval aggregates in
`w0/w1/w2`, `hfa14` in `s0..s3`, `hfa24` in `d4..d7`, and stack-passed
`hfa34` lanes loaded from the incoming stack area. The existing variadic
fixed-formal stack-home expectation was also corrected to assert bank-local
`s0`/`d1` sources instead of the old flat-index `s6`/`d7` sources.

`00204.c` is still not green, but the first bad fact is narrower. Generated
`fa4` no longer reads `s2` from `w5` or the first HFA lanes from `s1`/`d6` as
the authoritative incoming sources. The remaining `fa4` output still starts
with corrupted small-aggregate text and now prints `14.1 14.1 ... 0.0 0.0`,
with generated code showing shifted prepared parameter homes (`fmov s1, s0`;
`fmov s2, s1`; `fmov s3, s2`; `fmov s4, s3`), missing prepared homes for the
later `hfa24` lanes, and stack-passed `hfa34` `q` lanes copied into low frame
offsets that overlap the small-aggregate local bytes. That remaining owner is
not the caller publication path repaired in this packet.

## Suggested Next

Continue Step 2 by repairing the prepared parameter-home owner for AArch64
fixed formals with interleaved GPR/FPR pressure, including register-home
assignment for HFA lanes and stack-passed F128 HFA local homes. This likely
requires a packet that owns `src/backend/prealloc/**` in addition to the MIR
entry publication surface.

## Watchouts

The generated `arg -> fa4` caller remains coherent after this packet:
`ldrb w0` for `s1`, `s0..s3` for `hfa14`, `ldrh w1` for `s2`, `d4..d7` for
`hfa24`, `x2` for `s3`, and four outgoing stack `q` stores for `hfa34`.
The remaining corruption is on the callee entry/home side.

The current MIR entry-publication fix can move incoming sources into existing
prepared homes, but it cannot invent missing prepared homes for lanes that
prealloc failed to publish. Avoid treating a MIR-only register shuffle as the
full `fa4` fix unless prepared parameter homes are also repaired.

Untracked `review/*.md` files were present before this executor packet and were
left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded;
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and
`backend_aarch64_instruction_dispatch` passed, including the new mixed
GPR/HFA entry-formal coverage. `c_testsuite_aarch64_backend_src_00204_c`
still failed with `RUNTIME_NONZERO` / segmentation fault. The fresh first bad
output remains the fixed mixed `fa4` line, but generated-code evidence now
localizes the remaining fault to prepared parameter homes and stack-passed
F128 HFA home seeding rather than flat incoming source-register selection.
`test_after.log` is the fresh proof log for this packet and remaining blocker.
