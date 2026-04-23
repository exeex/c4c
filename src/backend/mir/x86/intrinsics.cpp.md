# `intrinsics.cpp` extraction

## Purpose and current responsibility

This file no longer implements x86 intrinsic lowering logic. Its current job is
to preserve a legacy source-path entry while redirecting responsibility to the
new lowering unit for atomics and intrinsics.

The file is executable evidence that the old ownership moved. It is not a
design surface and does not expose behavior beyond "include the real lowering
surface and keep the namespace anchor alive."

## Important APIs and contract surfaces

The only material surface is the include-level dependency handoff:

```cpp
#include "lowering/atomics_intrinsics_lowering.hpp"
```

The namespace body is intentionally empty except for a compatibility note:

```cpp
namespace c4c::backend::x86 {
// canonical lowering lives elsewhere
}
```

Contract implications:

- Consumers that still compile this translation unit expect the x86 backend
  intrinsic lowering surface to remain reachable from the legacy path.
- The actual behavior contract now lives in
  `lowering/atomics_intrinsics_lowering.*`, not here.
- This file should stay side-effect light; adding logic here would reintroduce
  split ownership.

## Dependency direction and hidden inputs

Dependency direction is one-way:

- `intrinsics.cpp` depends on `lowering/atomics_intrinsics_lowering.hpp`
- The real implementation dependency chain continues behind that header

Hidden input:

- Build-system or source-list compatibility. The file likely remains because
  some target still expects `intrinsics.cpp` to exist as a translation-unit
  path, even though semantic ownership moved.

## Responsibility buckets

- Compatibility shim: keep the old file path valid
- Ownership marker: document that canonical lowering moved
- Namespace anchor: preserve the backend namespace compilation surface

Not owned here:

- intrinsic selection logic
- atomic lowering rules
- code emission
- semantic dispatch policy

## Fast paths, compatibility paths, and overfit pressure

- Fast paths: none in this file
- Compatibility path: the whole file is a dormant compatibility layer
- Overfit pressure: high risk if future work starts adding one-off intrinsic
  logic here "because the file already exists"; that would recreate legacy
  split ownership and path-shaped behavior

## Rebuild ownership

This file should own only legacy-path compatibility, if it exists at all during
transition.

It should not own new lowering logic, special-case intrinsic handling, backend
policy, or any rebuilt API boundary. In a rebuild, canonical ownership belongs
in the dedicated lowering surface, and this file should either remain a trivial
shim or disappear entirely once build compatibility no longer requires it.
