# Storage Foundation API Explainer

## Authors

*   Emanuel Krivoy (fivedots@chromium.org)
*   Richard Stotz (rstz@chromium.org)
*   Jose Lopes (jabolopes@google.com, emeritus)

## Participate

*   [Issue Tracker](https://crbug.com/914488)
*   [Discussion forum](https://github.com/fivedots/storage-foundation-api-explainer/issues)

## What is this API about?

The web platform increasingly offers developers the tools they need to build
fined-tuned high-performance applications for the web. Most notably, WebAssembly
has opened the doors to fast and powerful web applications, while technologies
like Emscripten now allow developers to reuse tried and tested code on the web.
In order to truly leverage this potential, developers must be given the same
power and flexibility when it comes to storage. Other storage APIs often work
great for the use-cases they were explicitly built for, but covering new needs
through them often comes at a loss of performance and usability.

This is where Storage Foundation API comes in. Storage Foundation API is a new
fast and unopinionated storage API that  unlocks new and much-requested
use-cases for the web, such as implementing performant databases and gracefully
managing large temporary files.  With this new interface developers will be able
to “Bring their Own Storage” to the web, reducing the feature gap between web
and native. It will also complement the nascent ecosystem of performant-focused
Wasm applications with a storage backend that matches its needs.

Storage Foundation API is designed to resemble a very basic filesystem, aiming
to give developers flexibility  by providing generic, simple, and performant
primitives upon which they can build higher-level components. Applications can
take advantage of the best tool for their needs, finding the right balance
between usability, performance, and reliability.

A few examples of what could be done with Storage Foundation API:

*   Allow tried and true technologies to be performantly used as part of web
    applications e.g. using a port of your favorite storage library
within a website
*   Distribute a Wasm module for WebSQL, allowing developers to us it across
    browsers and opening the door to removing the unsupported API from Chrome
*   Allow a music production website to operate on large amounts of media, by
    relying on Storage Foundation API’s performance and direct buffered access
    to offload sound segments to disk instead of holding them in memory
*   Provide a persistent [Emscripten](https://emscripten.org/) filesystem that
    outperforms
[IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs)
and has a simpler implementation

Note: Storage Foundation API used to be called NativeIO. Some references to this
name still remain, especially in the description of the interface and in code
examples. They will be removed after the new name has landed on Chrome.

### Why does the web need another storage API?

The web platform offers a number of storage options for developers, each of them
built with specific use-cases in mind. Some of these options clearly do not
overlap with this proposal as they only allow very small amounts of data to be
stored (Cookies, [Web Storage
API](https://html.spec.whatwg.org/multipage/webstorage.html#webstorage)) or are
already deprecated for various reasons ([File and Directory Entries
API](https://developer.mozilla.org/en-US/docs/Web/API/File_and_Directory_Entries_API/Firefox_support),
[WebSQL](https://www.w3.org/TR/webdatabase/)). The [File System Access
API](https://wicg.github.io/native-file-system/) has a similar API surface, but
its main intended usage is to interface with the client’s filesystem and provide
access to data that may be outside of the origin’s or even the browser’s
ownership. This different focus comes with stricter security considerations and
higher performance costs.

[IndexedDB](https://w3c.github.io/IndexedDB/) can be used as a backend for some
of the Storage Foundation API’s use-cases. For example, Emscripten includes
[IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs),
an IndexedDB-based persistent file system. However, since IndexedDB is
fundamentally a key-value store, it comes with significant performance
limitations. Furthermore, directly accessing sub-sections of a file is even more
difficult and slower under IndexedDB.  Finally, the [Cache
API](https://developer.mozilla.org/en-US/docs/Web/API/Cache) is widely
supported, and is tuned for storing large-sized data such as web application
resources, but the values are immutable.

## Interface and examples

Note: We are currently exploring the tradeoffs between providing a synchronous
vs. asynchronous API. The following interfaces are designed to be asynchronous
as a temporary measure, they will be updated once a decision has been reached.

There are two main parts to the API:

*   File handles, which provides read and write access to an existing file
*   Filesystem calls, which provides basic functionality to interact with files
    and file paths

The following are our draft proposals for the interface, written in Web IDL.

### Filesystem calls

All filesystem calls are accessible through a global Storage Foundation
instance.


```webidl
// IMPORTANT: filenames are restricted to lower-case alphanumeric and
// underscore (a-z, 0-9, _).
interface NativeIOFileManager {
  // Opens the file with the given name if it exists and otherwise creates a
  // new file.
  [RaisesException] Promise<NativeIOFile> open(DOMString name);

  // Removes the file with the given name.
  [RaisesException] Promise<void> delete(DOMString name);

  // Returns all existing file names.
  [RaisesException] Promise<sequence<DOMString>> getAll();

  // Renames the file from old name to new name atomically.
  [RaisesException] Promise<void> rename(DOMString old_name, 
                                         DOMString new_name);
  
  // Requests new capacity (in bytes) for usage by the current execution
  // context. Returns the remaining amount of capacity available.
  [ CallWith = ScriptState, RaisesException ] Promise<unsigned long long>
  requestCapacity(unsigned long long requested_capacity);
  
  // Releases unused capacity (in bytes) from the current execution context.
  // Returns the remaining amount of capacity available.
  [ CallWith = ScriptState, RaisesException ] Promise<unsigned long long>
  releaseCapacity(unsigned long long requested_capacity);

  // Returns the capacity available for the current execution context.
  [ CallWith = ScriptState, RaisesException ] Promise<unsigned long long>
  getRemainingCapacity();
};
```

For this API we opted to have a minimal set of functions. This reduces the
surface area of the browser and simplifies the standardization process. More
advanced filesystem-like functionality can be implemented by client libraries,
as we've done with our prototype Emscripten filesystem.

### File handles

The following functions are accessible after opening a FileHandle object.


```webidl
dictionary NativeIOReadResult {
  // The result of transferring the buffer passed to read. It is of the same
  // type and length as source buffer.
  ArrayBufferView buffer;
  // The number of bytes that were succesfully read into buffer. This may be
  // less than the buffer size, if errors occur or if the read range spans
  // beyond the end of the file. It is set to zero if the read range is beyond
  // the end of the file.
  unsigned long long readBytes;
};

dictionary NativeIOWriteResult {
  // The result of transferring the buffer passed to write. It is of the same
  // type and length as source buffer.
  ArrayBufferView buffer;
  // The number of bytes that were successfully written into buffer. This may
  // be less than the buffer size if an error occurs.
  unsigned long long writtenBytes;
};

interface NativeIOFile {
  Promise<void> close();

  // Synchronizes (i.e., flushes) a file's in-core state with the storage
  // device.
  // Note: flush() might be slow and we are exploring whether offering a
  // faster, less reliable variant would be useful.
  [RaisesException] Promise<void> flush();

  // Returns the length of the file in bytes.
  [RaisesException] Promise<unsigned long long> getLength();

  // Sets the length of the file. If the new length is smaller than the current
  // one, bytes are removed starting  from the end of the file.
  // Otherwise the file is extended with zero-valued bytes.
  [RaisesException] Promise<void> setLength(unsigned long long length);

  // Reads the contents of the file at the given offset through a buffer that is
  // the result of transferring the given buffer, which is then left detached.
  // Returns a NativeIOReadResult with the transferred buffer and the the number
  // of bytes that were successfully read.
  [RaisesException] Promise<NativeIOReadResult> read(
                      ArrayBufferView buffer,
                      unsigned long long file_offset);

  // Writes the contents of the given buffer into the file at the given offset.
  // The buffer is transferred before any data is written and is
  // therefore left detached. Returns a NativeIOWriteResult with the transferred
  // buffer and the number of bytes that were successfully written. The file
  // will be extended if the write range spans beyond its length.
  //
  // Important: write only guarantees that the data has been written to the
  // file but it does not guarantee that the data has been persisted to the
  // underlying storage. To ensure that no data loss occurs on system crash,
  // flush must be called and it must successfully return.
  [RaisesException] Promise<NativeIOWriteResult> write(
                      ArrayBufferView buffer,
                      unsigned long long file_offset);
};
```

##### **Reads and writes**

Open a file, write to it, read from it, and close it to guarantee integrity of
the file’s contents.

```javascript
const file = await storageFoundation.open("test_file");  // opens a file (creating
                                                       // it if needed)
try {
  await storageFoundation.requestCapacity(100);  // request 100 bytes of
                                                 // capacity for this context.

  const writeBuffer = new Uint8Array([64, 65, 66]);
  // result.buffer contains the transferred buffer, result.writtenBytes is 3,
  // the number of bytes written. writeBuffer is left detached.
  let result =  await file.write(writeBuffer, 0);

  const readBuffer = new Uint8Array(3);
  // Reads at offset 1. result.buffer contains the transferred buffer,
  // result.readBytes is 2, the number of bytes read. readBuffer is left
  // detached.
  result = await file.read(readBuffer, 1);
  console.log(result.buffer);  // Uint8Array(3) [65, 66, 0]
} finally {
 file.close();
}
```

##### **List files**

Create/delete a few files and list them.

```javascript
await storageFoundation.open("sunrise");
await storageFoundation.open("noon");
await storageFoundation.open("sunset");
await storageFoundation.getAll();  // ["sunset", "sunrise", "noon"]

await storageFoundation.delete("noon");
await storageFoundation.getAll();  // ["sunrise", "noon"]
```

## Design Decisions

### Sync vs. Async

The decision to structure Storage Foundation API as a synchronous or
asynchronous API will come from the trade-off between performance and usability
vs. risks of blocking the execution flow.

WebAssembly execution is synchronous but technologies like Emscripten's
[Asyncify](https://emscripten.org/docs/porting/asyncify.html) allow modules to
call asynchronous functions. This comes at a
[cost](https://kripken.github.io/blog/wasm/2019/07/16/asyncify.html) though, a
50% increase to code size and execution time in the usual case and up to a 5x
increase in extreme cases like SQLite. Existing workarounds (like having to call
[FS.syncfs](https://emscripten.org/docs/api_reference/Filesystem-API.html#FS.syncfs)
from an asynchronous context) lead to weak persistence guarantees and to data
inconsistencies between tabs.

We are currently prototyping and profiling different ways of interfacing Wasm
modules with async code. We are also closely watching proposals for allowing
asynchronous functions in Wasm. This section will be updated with the results
and eventual design decision.

### Concurrent I/O

The current prototype of Storage Foundation API allows any file to be opened
only once. In particular, this means that files can not be shared between tabs.
Allowing a file to be opened multiple times requires significant overhead to
prevent concurrent, unsafe writes to the same file. Our current approach is
therefore quite safe yet restrictive. We may revisit this decision based on
developer feedback.

### Quota

Storage Foundation achieves fast and predictable performance by implementing its
own quota management system.

The web application must explicitly ask for capacity before storing any new
data. This request will be granted according to the browser's quota
guidelines. The application can then use the capacity to execute write
operations. The application can also delete or overwrite data to make space for
new data without issuing a new request.

This means that anytime an application starts a new Javascript execution context
(e.g., new tab, new worker, reloading the page), it must make sure it owns
sufficient capacity before writing new data. It must therefore either overwrite
existing files, delete/shrink old files or request additional capacity.

## Trying It Out

A prototype of Storage Foundation API is available in Chrome Canary. To enable
it, launch Chrome with the `-enable-blink-features=StorageFoundationAPI` flag or
enable “Experimental Web Platform feature” in
["chrome://flags"](chrome://flags).

Storage Foundation API will be available in an [origin
trial](https://web.dev/origin-trials/) starting with Chrome
90. [Sign up here to participate.](https://developer.chrome.com/origintrials/#/view_trial/2916080758722396161)

To make it easier to try the API, we’ve developed an [Emscripten
Filesystem](https://github.com/fivedots/storage-foundation-api-emscripten-fs)
and a
[tutorial](https://github.com/fivedots/storage-foundation-api-porting-tutorial)
with an example use-case.  We also provided a
[wrapper](https://github.com/fivedots/storage-foundation-api-async-wrapper) that
allows directly calling Storage Foundation API from C++ code.

## Security Considerations

Following the same pattern as other modern storage web APIs, access to Storage
Foundation API is origin-bound, meaning that an origin may only access
self-created data. It is also limited to secure contexts e.g. Storage Foundation
API is exposed to HTTPS- but not HTTP-served websites.

Storage quota will be used to distribute access to disk space and to prevent
abuse. Like other storage APIs, users must have the option of clearing the space
taken by Storage Foundation API through their user agents.

While no configuration information of the host is exposed through our interface,
care must be taken during implementation, such as not to expose
operating-system-specific behavior through the way file operations are executed.
