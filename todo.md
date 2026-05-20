Status: Active
Source Idea Path: ideas/open/337_aarch64_callee_saved_scalar_home_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Missing Callee-Saved Preservation

# Current Packet

## Just Finished

Step 1 localized the `00168` factorial residual to the prepared-to-AArch64
register-placement handoff, not liveness, regalloc, frame planning, or
prologue/epilogue emission.

Concrete evidence:

- `build/c4cll --codegen asm --target aarch64-unknown-linux-gnu
  tests/c/external/c-testsuite/src/00168.c` still emits `factorial` with
  `mov w19, w0`, `bl factorial`, then `mul w0, w19, w21`, while the prologue
  saves/restores only `x20`, `x21`, and `x30`.
- Focused prepared dump
  `build/c4cll --dump-prepared-bir --target aarch64-unknown-linux-gnu
  --mir-focus-function factorial --mir-focus-value p.i
  tests/c/external/c-testsuite/src/00168.c` shows `%p.i` is live across the
  recursive call and is represented in preservation authority:
  `preserve value=%p.i value_id=0 route=callee_saved_register save_index=0
  placement=gpr:callee_saved#0/w1 reg=x20 bank=gpr`.
- The same dump's prepared frame plan saves that authority as
  `saved_register bank=gpr placement=gpr:callee_saved#0/w1 reg=x20` plus
  `gpr:callee_saved#1/w1 reg=x21`, so frame planning and prologue inputs are
  carrying `x20`/`x21`.
- Source inspection with `c4c-clang-tool-ccdb` found the split authority:
  `src/backend/prealloc/target_register_profile.cpp:209` defines the prepared
  AArch64 callee-saved GPR pool as `x20`, `x21`, but
  `src/backend/mir/aarch64/abi/abi.cpp:381` defines codegen's AArch64 ABI
  callee-saved GPR array starting at `x19`, `x20`, `x21`, ...; placement
  conversion in `abi.cpp:220` maps `gpr:callee_saved#0/w1` through that ABI
  array, so emitted operands become `x19/w19` while the prepared frame plan
  still saves `x20`.

## Suggested Next

Start Step 2 with a semantic handoff repair: make AArch64 codegen's conversion
of prepared callee-saved placements use the same physical register authority
that prealloc/frame planning published, or otherwise derive both emitted
operands and saved-register frame records from one shared AArch64 prepared
callee-saved pool. Prove that `gpr:callee_saved#0/w1` no longer means `x20` to
the frame plan but `x19` to operand emission.

## Watchouts

Do not fix this by blindly saving `x19` in the frame while leaving the two
callee-saved placement maps divergent; that would preserve the narrow current
assembly but leave the prepared/frame handoff internally inconsistent. Also do
not reopen return-publication suppression: the recursive return reaches `x0`,
and the residual is the caller's `%p.i` home being emitted in a different
callee-saved register from the one the frame plan saves.

## Proof

`git diff --check` passed. Output captured in `test_after.log` (empty on
success).
