Status: Active
Source Idea Path: ideas/open/581_rv64_ordinary_floating_cast_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Ordinary Cast Owners

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by refreshing the retained RV64 ordinary
floating-cast representatives under
`build/agent_state/581_rv64_ordinary_floating_cast_lowering/step1/`.
`cmake --build --preset default --target c4cll` passed, prepared dumps returned
0, and object-route attempts returned 1 with the current first owner still
`unsupported_floating_cast`.

Owner facts:
- `src/920618-1.c`: first owner is RV64 object `CastInst` lowering for
  `%t0 = bir.fptrunc double 0x3FF199999999999A to float`; source is a double
  immediate, destination is float, result home is FPR `ft0`; no chained cast
  before the owner.
- `src/ieee/pr67218.c`: first owner is RV64 object `CastInst` lowering for
  `%t1 = bir.uitofp i32 %t0 to float` followed by
  `%t2 = bir.fpext float %t1 to double`; operand `%t0` is in GPR `t0`, `%t1`
  is in FPR `ft0`, and `%t2` is in FPR `fs1`; this is a chained int-to-F32
  plus F32-to-F64 route, with an additional F32 zero-immediate-to-F64 compare
  operand in `main`.
- `src/pr23941.c`: first owner is RV64 object `CastInst` lowering for
  `%t0 = bir.fptrunc double 0x4000000000000000 to float`; the row later
  includes `%t2 = bir.fpext float %t1 to double`; `%t0` and `%t2` are in FPR
  `ft0`, `%t1` is in FPR `fs1`; this is a chained ordinary F64-to-F32,
  F32-arithmetic, F32-to-F64 route.

The F128/long-double quarantine remains unchanged: `src/20040709-1.c` and
`src/ieee/20011123-1.c` stayed outside this proof and were not used to justify
the ordinary F32/F64 lane.

## Suggested Next

Execute `plan.md` Step 2 by adding focused RV64 object-emission coverage for
semantic ordinary cast forms: constant F64-to-F32 `fptrunc`, FPR-register
F32-to-F64 `fpext`, and the retained `uitofp i32 to float` chain if the focused
fixture can cover it without row-name matching.

## Watchouts

- Keep this lane limited to ordinary F32/F64 casts and the retained
  `uitofp i32 to float` evidence if it still shares the same owner.
- The current route diagnostic already says some prepared FPR width casts and
  I32/I64-to-F32/F64 integer-to-floating casts are supported; Step 2 should
  target the missing operand/home combinations proven above rather than weaken
  that diagnostic.
- Do not use F128, long-double, soft-float helper, scalar compare, or variadic
  helper work as justification for this idea.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or residual filename
  shortcuts.

## Proof

Proof output is in `test_after.log`.

- `cmake --build --preset default --target c4cll`: pass.
- Fresh prepared dumps for `src/920618-1.c`, `src/ieee/pr67218.c`, and
  `src/pr23941.c`: all return code 0.
- Fresh RV64 object-route attempts for those three representatives: all return
  code 1 with `unsupported_floating_cast`, confirming current owner facts for
  this classification step.
