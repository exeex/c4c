# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.20
Current Step Title: Receive the selected `LirMemcpyOp` authority row

## Just Finished

- Step 7.19 complete: accepted builtin-popcount `Ctpop` i32 direct and i64
  Trunc-to-i32 final-use receipt (`565be6932`). Its fresh build, focused 2/2,
  matching regression guard, and broader backend proof are historical evidence;
  do not repeat that row.
- Closed idea 748 published the sole next receiver-ready memcpy producer
  contract in `6a12cddab` and `dac9c8f81`, with focused backend 5/5,
  monotonic canonical regression guard, and broader full-suite 3034/3034
  proof.

## Suggested Next

- Execute Step 7.20 only: receive the exact closed-748 selected non-volatile
  fixed-aggregate byval `LirMemcpyOp::selected_authority` into typed Raw-BIR,
  importer dispatch, reachable verification, and transactional coverage.

## Watchouts

- The only semantic inputs are the structured destination/source value IDs,
  i64 immediate size, object IDs/owners, and live-site facts. Do not parse or
  compare display operands. All unselected memcpy rows and other memory/object
  families remain unsupported and fail-closed.

## Proof

- Step 7.20 proof must include a fresh build, focused receiver coverage plus
  the selected producer authority regression neighbor, then supervisor-owned
  matching regression and broader checkpoint. Do not reuse closed-748 proof as
  receiver proof.
