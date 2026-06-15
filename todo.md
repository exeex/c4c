# Current Packet

Status: Active
Source Idea Path: ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map target public-field consumers

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
from the source idea. No implementation, tests, build outputs, or regression
logs were touched.

## Suggested Next

Execute Step 1 from `plan.md`: map x86/riscv direct reads or legitimate
candidate reads of `PreparedFunctionLookups::memory_accesses`, compare the
consumer shape against the out-of-scope AArch64 scalar ALU example only as
reference, and identify the narrow fixture path plus proof command for the next
packet.

## Watchouts

- Do not edit implementation or tests before the target backend consumer and
  fixture route are named.
- Do not claim helper-only, oracle-only, edge-publication, or addressing-record
  evidence as backend `memory_accesses` public-field consumer coverage.
- Do not use hand-built stale prepared state or test-only mutation that
  bypasses normal prepared lookup construction.
- Do not add named-fixture shortcuts, branch-specific matching, or
  testcase-shaped logic.
- Preserve the adjacent x86 `branch_join_loadlocal_then_add` compatibility
  output unless the supervisor accepts a reviewed contract change.
- Do not weaken unsupported status, helper/oracle status, fallback behavior,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baselines.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
