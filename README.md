# NativeIO API

## Authors:

* Emanuel Krivoy (krivoy@google.com)
* Jose Lopes (jabolopes@google.com)

## Participate

* Issue Tracker: https://crbug.com/914488
* Discussion forum: https://github.com/fivedots/nativeio-explainer

## Table of Contents

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->


- [Introduction](#introduction)
- [Requirements](#requirements)
- [Interface and Examples](#interface-and-examples)
  - [Filesystem Calls](#filesystem-calls)
  - [File Handles](#file-handles)
  - [Examples](#examples)
    - [Reads and writes](#reads-and-writes)
    - [List files by prefix](#list-files-by-prefix)
    - [Rename and unlink](#rename-and-unlink)
- [Design Decisions](#design-decisions)
  - [Sync vs Async](#sync-vs-async)
- [Prototypes](#prototypes)
- [Security Considerations](#security-considerations)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Introduction

The web platform offers a number of APIs that give developers the ability to
store and retrieve data. These APIs are generally high-level and opinionated,
providing an interface that's well-tuned for certain use cases, but ill-suited
to others. For example, WebSQL implements a relational database; as long as your
processing needs are met by the exact SQL dialect supported in SQLite 3.6.19
(released almost exactly a decade ago), it might be right for you! If, on the
other hand, you prefer a different dialect, or simply want to represent your
data in a way that doesn't fit the existing storage mechanisms, you're a bit out
of luck.

Native platforms have taken a different approach to storage APIs, aiming to give
the developer community flexibility by providing generic, simple, and performant
primitives upon which developers can build higher-level components. Applications
can take advantage of the best tool for their needs, finding the right balance
between API, performance, and reliability. It would be ideal if the web could
support the same freedom of choice.

WebAssembly seems like a path towards that vision. Developers can bring their
own storage mechanism, compiling any native tool into a WASM module, and binding
it to the web APIs discussed above. This works today, but the result is not
quite ready for production. For example, the
[IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs)
backend provided by Emscripten has consistency issues between tabs, and slow
performance, both due in part to the mismatch between the requirements of WASM
and the design objectives of the backing store.

NativeIO is a new low-level storage API that resembles a very basic filesystem.
It is extremely performant, showing great improvement in prototype benchmarks.
With this new interface developers will be able to “Bring their Own Storage” to
the web, reducing the feature gap between web and native.

A few examples of what would be possible after NativeIO exists:

*   Allow tried and true technologies to be performantly used as part of web
    applications e.g. using a port of your favorite database within a website
*   Distribute a WASM module for WebSQL, allowing developers to us it across
    browsers and opening the door to removing the unsupported API from Chrome
*   Provide a persistent [Emscripten](https://emscripten.org/) filesystem that
    outperforms
    [IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs)
    and has a simpler implementation

## Requirements

Our main objective is to close the feature gap that exists between web and
native by giving developers a broader storage choice. NativeIO provides the
storage primitives needed to effectively run higher level storage in the client
side.

NativeIO must be generic and performant enough for developers to implement or
port their preferred storage component and use it as part of their web
application. It should also be able to function as a backend for existing web
storage APIs. Since WebAssembly is the key technology needed to port native
technologies, NativeIO must be able to performantly interface with WASM modules.

## Interface and Examples

> Note: We are currently exploring the tradeoffs between providing a synchronous
> vs. asynchronous API. The following interfaces are designed to be synchronous
> as a temporary measure, they will be updated once a decision has been reached.

There are two main parts to the API:

*   File handles, which provides read and write access to an existing file
*   Filesystem calls, which provides basic functionality to interact with files
    and file paths

The following are our draft proposals for the interface, written in Web IDL.
Since this is a synchronous API, it will only be accessible to web and shared
workers. That way we are not adding blocking functionality to the main thread.

### Filesystem Calls

All filesystem calls are accessible through a global NativeIO instance.

```webidl
// IMPORTANT: filenames are restricted to lower-case alphanumeric and underscore
// (a-z, 0-9, _). This restricted character set should work on most filesystems
// across Linux, Windows, and Mac platforms, because some systems, e.g., treat
// “.” with a special  meaning, or treat lower-case and upper-case as the same
// file, etc.
interface NativeIO {
    // Opens the file with the given name.
    [RaisesException] FileHandle openFile(DOMString name);

    // Create the file with the given name if it doesn't exist.
    [RaisesException] void createFile(DOMString name);

    // Returns the existing file names that have the given prefix
    [RaisesException] sequence<DOMString> listByPrefix(DOMString namePrefix);

    // Renames the file from old name to new name atomically.
    [RaisesException] void rename(DOMString oldName, DOMString newName);

    // Unlinks the file from the filesystem.
    [RaisesException] void unlink(DOMString name);
};
```

For this API we opted to have a minimal set of functions. This reduces the
surface area of the browser and (hopefully) simplifies the standardization
process. More advanced filesystem-like functionality can be implemented by
client libraries, as we've done with our prototype Emscripten filesystem.

### File Handles

The following functions are accessible after opening a FileHandle object.

```webidl
[
    Constructor
] dictionary FileAttributes {
    // Size of the file in bytes.
    attribute unsigned long long size;
};

interface FileHandle {
    // Returns the file attributes.
    [RaisesException] FileAttributes getAttributes();

    // Sets the file attributes. The API does not guarantee that different
    // attributes will be updated atomically because of limitations in the
    // underlying file systems on different platforms.
    [RaisesException] void setAttributes(FileAttributes attributes);

    // Reads the contents of the file at the given offset into the given buffer. The number of
    // bytes read is the size of the buffer. Returns the number of bytes read. This may return
    // less bytes than those requested if the read range spans over the end of the file.
    // Returns 0 if the read range is beyond the end of the file. An exception is raised if there
    // is a read failure.
    [RaisesException] unsigned long read(BufferSource buffer, unsigned long long offset);

    // Writes the given buffer at the given file offset. Returns the number of bytes written.
    // If the write range spans over the end of the file, the file will be resized accordingly.
    // Returns the number of bytes written to the file. The number of bytes written can be
    // less than those requested, for example, because the disk is full. The number of bytes
    // written can never be 0. An exception is raised if there is a write failure.
    [RaisesException] unsigned long write(BufferSource buffer, unsigned long long offset);

    // Synchronizes (i.e., flushes) a file’s in-core state with the storage device.
    //
    // Important: write only guarantees that the data has been written to the file
    // but it does not guarantee that the data has been persisted to the
    // underlying storage. To ensure that no data loss occurs on system crash,
    // sync must be called and it must return successfully without raising an
    // exception.
    [RaisesException] void sync();

    // Flushes the files and closes the file handle.
    //
    // This always closes the file handle even if an error is thrown. This must
    // be called at least once to avoid leaking resources. Calling this when the
    // file is already closed is a no-op.
    [RaisesException] void close();
};
```

### Examples

#### Reads and writes

```javascript
handle = io.openFile("hello-world") // opens a file (creating it if needed)
try {
  writeBuffer = new Int8Array([3, 1, 4])
  handle.write(writeBuffer, 0) // returns 3, the number of bytes written

  readBuffer = new Int8Array(3) // creates a new array of length 3
  handle.read(readBuffer, 0, 3) // returns 3, the number of bytes read

  console.log(readBuffer) // Int8Array(3) [1, 2, 3]
} finally {
  handle.close()
}
```

#### List files by prefix

```javascript
// Create the files
io.createFile("sunrise")
io.createFile("noon")
io.createFile("sunset")

io.listByPrefix("sun") // ["sunset", "sunrise"]

// the empty string matches all existing files
io.listByPrefix("") // ["sunset", "sunrise", "noon"]
```

#### Rename and unlink

```javascript
io.createFile("file")
io.rename("file", "newfile")
io.unlink("newfile")
```

## Design Decisions

### Sync vs Async

The decision to structure NativeIO as a synchronous or asynchronous API will
come from the trade-off between performance and usability vs. risks of blocking
the execution flow.

WebAssembly execution is synchronous but technologies like Emscripten's
[Asyncify](https://emscripten.org/docs/porting/asyncify.html) allow modules to
call asynchronous functions. This comes at a cost though, a 50% increase to code
size and execution time in the usual case and up to a 5x increase in extreme
cases like SQLite
([source](https://kripken.github.io/blog/wasm/2019/07/16/asyncify.html)).
Existing workarounds (like having to call
[FS.syncfs](https://emscripten.org/docs/api_reference/Filesystem-API.html#FS.syncfs from an asynchronous context)
lead to weak persistence guarantees and to data inconsistencies between tabs.

We are currently prototyping and profiling different ways of interfacing WASM
modules with async code. This section will be updated with the results and
eventual design decision.

## Prototypes

_TODO: Link demos and code once they are easily shareable_

_TODO: Link benchmark results_

In order to discover the functional requirements and validate this API, we
decided to port [SQLite](https://www.sqlite.org/index.html) and
[LevelDB](https://github.com/google/leveldb) to the web. They were chosen since
they are the backends of two existing web APIs, and

In order to run the databases as WebAssembly modules, we decided to compile them
with [Emscripten](https://emscripten.org/). This led to us adding a new
Emscripten filesystem that uses NativeIO to emulate more complex functionality.
Both candidate libraries were successfully run and showed significant
performance improvements when compared to the same libraries running on top of
the
[Chrome filesystem](https://developer.mozilla.org/en-US/docs/Web/API/Window/requestFileSystem).

## Security Considerations

The API will have similar security policies to the one used in modern web
storage APIs. Access to files will be isolated per origin. Quota checks will be
used to prevent misuse of user resources.
