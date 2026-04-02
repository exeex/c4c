# Std Vector Success-Path And STL Throughput Follow-On

Status: Open
Last Updated: 2026-04-02

## Goal

Make the motivating `std::vector` / library-header validation path usable when
parser work succeeds or takes a long time, without reopening the completed
parser-debug observability runbook.

## Why This Idea Exists

The completed parser-debug surface runbook established better failure-local
output, tentative lifecycle tracing, injected-token markers, and narrower CLI
flags. During Step 5 validation:

- `tests/cpp/eastl/eastl_vector_simple.cpp` reached a real parser failure and
  benefited from the new failure-local and tentative output once invoked with
  `-I ref/EASTL/include -I ref/EABase/include/Common`.
- `tests/cpp/std/std_vector_simple.cpp` did not finish `--parse-only` within a
  60s timeout in this environment, so the main remaining obstacle there is not
  missing trace payload.

## Concrete Problems

- There is still no high-signal success-path parser summary for large STL-style
  parses.
- `std_vector_simple.cpp` currently exceeds a practical manual validation
  window, which makes parser bring-up harder even when there is no immediate
  parse failure.
- Manual EASTL validation still depends on explicit repo-local include flags.

## Candidate Directions

- Add a success-path parser-debug summary mode for large successful parses.
- Investigate where `std_vector_simple.cpp` time is spent: preprocessing,
  tokenization, parser recursion, or debug bookkeeping.
- Decide whether motivating library workflows should gain checked wrapper
  scripts or tests that supply the required include paths automatically.

## Non-Goals

- Do not reopen the completed failure-local / tentative / injected trace slice.
- Do not silently broaden into general parser optimization work without a
  concrete measurement target.
