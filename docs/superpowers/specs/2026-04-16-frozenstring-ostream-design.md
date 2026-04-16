# FrozenString ostream support design

## Problem

`FrozenString` already works with `std::format`, but it cannot be passed directly to `std::cout` or `std::cerr` with `operator<<`. Users currently need to call `.sv()` or convert manually, which is inconsistent with the rest of the public API.

## Goal

Add direct narrow-stream output support so `FrozenString` can be written with:

```cpp
std::cout << value;
std::cerr << value;
```

## Non-goals

- Wide stream support such as `std::wostream`
- New formatting options beyond the stream's existing behavior
- New helper APIs such as `print()` wrappers

## Chosen approach

Add a non-member `operator<<` overload in the public header surface for `frozenchars::FrozenString<N>` and target narrow streams only.

### API shape

```cpp
template <size_t N>
auto operator<<(std::ostream& os, frozenchars::FrozenString<N> const& value)
  -> std::ostream&;
```

### Behavior

- Output only the visible string contents represented by `value.length`
- Delegate to the existing `std::string_view` representation via `value.sv()`
- Follow the same formatted insertion semantics as `std::string_view`, including width,
  fill, and alignment behavior provided by the stream
- Return the original stream reference to support chaining

## Why this approach

1. It matches the user-facing goal exactly: direct use with `std::cout` and `std::cerr`
2. It stays consistent with the existing `std::format` integration, which also exposes the string view contents
3. It avoids unnecessary copies and keeps the implementation small and cross-compiler friendly

## Rejected alternatives

### `std::basic_ostream<char, Traits>` template overload

This is slightly more generic, but it does not add meaningful value for the current narrow-stream-only requirement.

### Helper function instead of `operator<<`

This would not satisfy direct `std::cout << value` usage.

## Implementation notes

- Place the overload close to the `FrozenString` definition so it is reachable from the main public header
- Include the minimal standard header required for `std::ostream`
- Keep the overload in the same namespace as `FrozenString` so unqualified stream insertion finds it naturally

## Test plan

Add runtime tests using `std::ostringstream` to verify:

1. A normal `FrozenString` is written as its visible contents
2. An empty `FrozenString` writes nothing
3. A `FrozenString` whose internal buffer contains characters past `length` only writes the prefix selected by `length`
4. Width and fill formatting behave the same way as ordinary string insertion

`std::ostringstream` is sufficient because `std::cout` and `std::cerr` use the same `std::ostream` insertion contract.
