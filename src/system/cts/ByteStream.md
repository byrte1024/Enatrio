# ByteStream -- Cursor-Based Byte Buffer

## Overview

ByteStream is a growable byte buffer with a sequential cursor for reading and
writing binary data. It supports auto-growing writes, bounds-checked reads,
hex/binary/ASCII output, and portable file I/O via the `.ebsf` format.

## Include

```c
#include "system/cts/ByteStream.h"
```

---

## Creating and Destroying

```c
ByteStream *stream = ByteStream_Create(64);  // initial capacity in bytes
// ...
ByteStream_Destroy(stream);                  // frees buffer and struct
```

`ByteStream_Create` allocates the struct and an internal buffer. If `capacity`
is 0, it is promoted to 1. Returns `NULL` on allocation failure.

`ByteStream_Destroy` frees both the data buffer and the struct.

---

## Writing

### ByteStream_Write

```c
int ByteStream_Write(ByteStream *stream, const void *data, uint32_t size);
```

Copies `size` bytes from `data` into the buffer at the current cursor position.
The cursor advances by `size`. If the write would exceed capacity, the buffer
auto-grows (doubling until sufficient). Returns 0 on success, -1 on overflow or
allocation failure.

Writes overwrite bytes at the cursor position -- they do not insert. If the
cursor is in the middle of existing data, the bytes at that position are
replaced.

### ByteStream_WriteValue

```c
ByteStream_WriteValue(stream, type, value)
```

Macro shorthand for writing a single typed value:

```c
ByteStream_WriteValue(stream, int32_t, 42);
ByteStream_WriteValue(stream, float, 3.14f);
ByteStream_WriteValue(stream, uint8_t, 0xFF);
```

Expands to `ByteStream_Write(stream, &(type){value}, sizeof(type))`.

---

## Reading

### ByteStream_Read

```c
int ByteStream_Read(ByteStream *stream, void *out, uint32_t size);
```

Copies `size` bytes from the buffer at the current cursor position into `out`.
The cursor advances by `size`. Returns 0 on success, -1 if the read would go
past `length`.

### ByteStream_ReadDeref

```c
type val = ByteStream_ReadDeref(stream, type);
```

Reads `sizeof(type)` bytes at the cursor and returns them as a value of the
given type. Advances the cursor. If the read would exceed `length`, returns a
zero-initialized value of that type.

```c
int32_t x = ByteStream_ReadDeref(stream, int32_t);
float   f = ByteStream_ReadDeref(stream, float);
```

### ByteStream_PeekDeref

```c
type val = ByteStream_PeekDeref(stream, type);
```

Like `ByteStream_ReadDeref`, but does **not** advance the cursor. Returns a
zero-initialized value if the peek would exceed `length`.

---

## Cursor Control

| Function / Field | Description |
|---|---|
| `ByteStream_Seek(stream, position)` | Sets the cursor to `position`. Clamped to `length` if out of range. |
| `ByteStream_Skip(stream, bytes)` | Advances the cursor by `bytes`. Clamped so it never exceeds `length`. |
| `ByteStream_Rewind(stream)` | Resets the cursor to 0. |
| `ByteStream_Remaining(stream)` | Returns `length - cursor` (bytes left to read). |
| `ByteStream_Clear(stream)` | Resets both `length` and `cursor` to 0. Does not free or shrink the buffer. |

---

## Print / Log

Six output functions display the buffer contents in different formats. The
`Print` variants use `printf`; the `Log` variants use `LOG_INFO`.

| Function | Format |
|---|---|
| `ByteStream_PrintHex(stream)` | Hex dump, 16 bytes per line |
| `ByteStream_PrintBinary(stream)` | Binary dump, 8 bytes per line |
| `ByteStream_PrintAscii(stream)` | Printable ASCII (non-printable shown as `?`) |
| `ByteStream_LogHex(stream)` | Same as PrintHex, via LOG_INFO |
| `ByteStream_LogBinary(stream)` | Same as PrintBinary, via LOG_INFO |
| `ByteStream_LogAscii(stream)` | Same as PrintAscii, via LOG_INFO |

All variants print a header line showing `[length/capacity bytes, cursor=N]`.

---

## File I/O

### SaveToFile

```c
int ByteStream_SaveToFile(ByteStream *stream, const char *path);
```

Writes the buffer to a file in `.ebsf` format. Returns 0 on success, -1 on
failure (logged).

### LoadFromFile

```c
ByteStream *ByteStream_LoadFromFile(const char *path);
```

Reads an `.ebsf` file and returns a new `ByteStream` with cursor at 0.
Returns `NULL` on failure (logged). Rejects files whose data length exceeds
`BYTESTREAM_MAX_SIZE`.

### .ebsf File Format

```
Offset  Size  Description
------  ----  -----------
0       4     Magic bytes: "EBSF" (Enatrio ByteStream File)
4       4     Format version (uint32_t, little-endian) -- currently 1
8       4     Data length in bytes (uint32_t, little-endian)
12      N     Raw data bytes
```

Little-endian encoding is written/read byte-by-byte, so .ebsf files are
portable across platforms regardless of host endianness.

---

## Security

- **BYTESTREAM_MAX_SIZE** -- defaults to 1 GB (`1024 * 1024 * 1024`). Can be
  overridden by defining the macro before including the header. Both `_Grow`
  and `LoadFromFile` enforce this cap.

- **Overflow-safe arithmetic** -- `_ByteStream_WouldOverflow` checks for
  uint32_t wraparound before any `cursor + size` addition. Write and Read
  both call this before proceeding.

- **Bounds-checked reads** -- `ByteStream_Read` and `_ByteStream_ReadInPlace`
  return failure (-1 or NULL) if `cursor + size > length`. No out-of-bounds
  memory access occurs.

- **File validation** -- `LoadFromFile` validates the magic bytes, format
  version, and data length before allocating or reading any data. Truncated
  files are detected and rejected at every stage.

---

## API Reference

| Function / Macro | Signature | Returns |
|---|---|---|
| `ByteStream_Create` | `(uint32_t capacity)` | `ByteStream*` or `NULL` |
| `ByteStream_Destroy` | `(ByteStream *stream)` | `void` |
| `ByteStream_Write` | `(ByteStream*, const void*, uint32_t)` | `int` (0 or -1) |
| `ByteStream_WriteValue` | `(stream, type, value)` | `int` (0 or -1) |
| `ByteStream_Read` | `(ByteStream*, void*, uint32_t)` | `int` (0 or -1) |
| `ByteStream_ReadDeref` | `(stream, type)` | `type` (zero on failure) |
| `ByteStream_PeekDeref` | `(stream, type)` | `type` (zero on failure) |
| `ByteStream_Seek` | `(ByteStream*, uint32_t)` | `void` |
| `ByteStream_Skip` | `(ByteStream*, uint32_t)` | `void` |
| `ByteStream_Rewind` | `(ByteStream*)` | `void` |
| `ByteStream_Remaining` | `(ByteStream*)` | `uint32_t` |
| `ByteStream_Clear` | `(ByteStream*)` | `void` |
| `ByteStream_PrintHex` | `(ByteStream*)` | `void` |
| `ByteStream_PrintBinary` | `(ByteStream*)` | `void` |
| `ByteStream_PrintAscii` | `(ByteStream*)` | `void` |
| `ByteStream_LogHex` | `(ByteStream*)` | `void` |
| `ByteStream_LogBinary` | `(ByteStream*)` | `void` |
| `ByteStream_LogAscii` | `(ByteStream*)` | `void` |
| `ByteStream_SaveToFile` | `(ByteStream*, const char*)` | `int` (0 or -1) |
| `ByteStream_LoadFromFile` | `(const char*)` | `ByteStream*` or `NULL` |

---

## Struct

```c
typedef struct ByteStream {
    uint8_t  *data;      // heap-allocated buffer
    uint32_t  capacity;  // allocated size of data
    uint32_t  length;    // number of bytes written (high-water mark)
    uint32_t  cursor;    // current read/write position
} ByteStream;
```

---

## Contract

- **Auto-grow on write.** Writes that exceed capacity cause the buffer to
  double until sufficient (capped at `BYTESTREAM_MAX_SIZE`). The caller does
  not need to manage capacity.

- **Reads past length return failure.** `ByteStream_Read` returns -1.
  `ByteStream_ReadDeref` returns a zero-initialized value. No out-of-bounds
  access occurs.

- **Seek clamps to length.** `ByteStream_Seek` with a position beyond
  `length` sets the cursor to `length`, not beyond it.

- **Writes overwrite, not insert.** Writing at a cursor in the middle of
  existing data replaces bytes at that position. There is no insert mode.

- **Length is a high-water mark.** `length` tracks the furthest byte ever
  written. Writing at a cursor before `length` does not shrink it. Only
  `ByteStream_Clear` resets `length`.
