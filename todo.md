Status: Active
Source Idea Path: ideas/open/430_rv64_integer_div_rem_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Div/Rem Evidence And Choose Packet

# Current Packet

## Just Finished

Activated `ideas/open/430_rv64_integer_div_rem_lowering.md` as the next active
RV64 post-contract plan. Selection rationale: after closing 429, the original
follow-up ordering names integer div/rem as the next clear implementation
candidate; it is a pure RV64 integer instruction owner, while 431 and 433 are
producer-boundary or residual classification routes.

## Suggested Next

Execute Step 1: audit current RV64 scalar binary lowering and representative
integer div/rem evidence, then decide whether the first packet is capability
repair, focused coverage/probe validation, or close-readiness cleanup for
existing support.

## Watchouts

- Do not use divisor-specific handling, representative filenames, or row-shaped
  shortcuts.
- Preserve signedness: `sdiv`/`srem` must not route through unsigned behavior,
  and `udiv`/`urem` must not route through signed behavior.
- Keep floating-point division, F128/I128 helper work, pointer/address,
  select/join, aggregate ABI, call publication, and global memory residuals
  outside this plan.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
