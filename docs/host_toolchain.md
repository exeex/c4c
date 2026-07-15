# Host Toolchain Contract

The c4c implementation and its in-tree native tests require strict,
non-extension C++20. The top-level [`CMakeLists.txt`](../CMakeLists.txt) is the
single authority for that requirement: it selects C++20 for the project and
disables compiler-specific language extensions.

This is a host-build contract. It does not select the C or C++ language mode
accepted by c4cll, and it does not replace language-under-test flags used by
compiler fixtures or driver workflows.

## Configuration contract

The minimum supported CMake version remains 3.20. During configuration, the
root build probes the selected C++ compiler and standard library for all of the
following:

- `__cplusplus >= 202002L`;
- C++20 designated initialization;
- `consteval`; and
- `std::span`.

Configuration fails immediately when any probe is unavailable. The diagnostic
reports the selected compiler ID and version and identifies the missing
capability. Compiler support is therefore capability-gated: do not infer that
an untested compiler family or version is supported merely because it offers a
nominal C++20 mode.

The verified acceptance host for this migration is:

- Clang 22.1.7;
- libc++ 220107; and
- arm64 Darwin.

That record is evidence for this exact host, not a portability claim for other
compiler or standard-library combinations.

## Adding an in-tree target

Production and native-test targets created under the root project inherit the
root C++20 authority. Do not add a target-local `CXX_STANDARD`,
`CXX_EXTENSIONS`, `cxx_std_*` compile feature, or manual `-std=` option. A local
standard pin would recreate competing authorities and could let a new target
silently diverge from the rest of the implementation.

Third-party build boundaries remain separate. An imported, vendored, or
externally configured dependency may retain its own standard policy when that
boundary is explicit; it must not weaken the standard selected for c4c-owned
targets.

Likewise, flags that choose the source language exercised by c4cll are test or
compiler-input policy, not host compilation policy. Keep those flags local to
the language-under-test workflow.
