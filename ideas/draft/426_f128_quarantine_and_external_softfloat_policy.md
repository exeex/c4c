# F128 Quarantine And External Softfloat Policy

Status: Open
Type: Low-priority policy and quarantine idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: F128 feature policy
Reference Branch: `try_gcc_torture`

## Goal

Define how RV64 F128 work should be quarantined, feature-gated, or postponed so
it does not dominate RV64 gcc_torture progress or contaminate ordinary scalar,
call, local-memory, and prepared-carrier paths.

The immediate policy is screening first: when a gcc_torture row is primarily
F128, classify it into this quarantine lane and keep it out of the ordinary-C
repair queue unless fresh evidence proves it also blocks a broad non-F128
capability.

## Why This Exists

The `try_gcc_torture` branch spent a disproportionate amount of work on F128
and still reduced broad RV64 gcc_torture pass count. F128 is a niche feature
relative to ordinary C coverage. When eventually supported, it should use
external soft-float helpers; the compiler should only provide minimal ABI glue
and complete prepared facts.

F128 has low project value relative to ordinary C coverage. The project should
prefer explicit fail-closed/quarantine testcase handling over spending near-term
implementation effort on F128 arithmetic, F128 conversion, or `conversion.c`
local progress.

## In Scope

- Decide where F128 should fail closed on reset `main`.
- Define the primary-F128 testcase screening rule for gcc_torture rows and how
  those rows are excluded from ordinary-C progress accounting.
- Identify any existing or future F128 code that must be isolated behind clear
  predicates or feature gates.
- Define the minimal acceptable F128 model: external soft-float helper calls,
  explicit ABI lane facts, and no Q-extension assumptions.
- Record which `try_gcc_torture` F128 lessons are useful but postponed.

## Out Of Scope

- Implementing F128 support as part of the main RV64 recovery route.
- Implementing softfloat arithmetic inside the compiler.
- Letting `conversion.c` drive the next umbrella.
- Treating F128 testcase pass count as useful near-term progress.
- Reworking ordinary scalar/FPR, local-memory, call ABI, or stack-frame paths
  for F128 convenience.

## Acceptance Criteria

- F128 is explicitly marked lowest priority in the RV64 gcc_torture plan.
- Primary-F128 gcc_torture testcase rows are explicitly classified as
  quarantine/unsupported-first rows and are not counted as ordinary-C repair
  blockers.
- Any future F128 implementation criteria require external helper ABI glue,
  feature isolation, fail-closed malformed cases, and default CTest stability.
- The policy identifies which `try_gcc_torture` F128 changes are not safe to
  replay wholesale.
- No implementation changes are required to close this policy idea unless the
  reviewer requests a small fail-closed gate.

## Reviewer Reject Signals

- Reject any plan that makes F128 the primary RV64 gcc_torture route.
- Reject any plan that keeps primary-F128 testcase rows in the ordinary-C
  implementation queue instead of screening them into F128 quarantine first.
- Reject compiler-internal softfloat arithmetic implementation as scope creep.
- Reject Q-extension assumptions or FLQ/FSQ-style full-width F128 hardware
  memory operations for the LP64D route.
- Reject F128 changes that alter ordinary scalar/FPR or call ABI behavior
  without a non-F128 bucket justification.
- Reject `conversion.c`-only proof as sufficient broad progress.
