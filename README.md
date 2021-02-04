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

Note: Storage Foundation API used to be called NativeIO, and the Javascript
object still has this name. We will update the explainer once the new name is
available in Chrome.

### Filesystem calls

All filesystem calls are accessible through a global NativeIO instance.


```webidl
// IMPORTANT: filenames are restricted to lower-case alphanumeric and
// underscore (a-z, 0-9, _).
interface NativeIOManager {
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
};
```

For this API we opted to have a minimal set of functions. This reduces the
surface area of the browser and simplifies the standardization process. More
advanced filesystem-like functionality can be implemented by client libraries,
as we've done with our prototype Emscripten filesystem.

### File handles

The following functions are accessible after opening a FileHandle object.


```webidl
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

  // Reads the contents of the file at the given offset into the given buffer.
  // Returns the number of bytes that were successfully read. This may be less
  // than the buffer size, if errors occur or if the read range spans
  // beyond the end of the file. Returns zero if the read range is beyond the
  // end of the file.
  [RaisesException] Promise<unsigned long long> read(
                      [AllowShared]  ArrayBufferView buffer,
                      unsigned long long file_offset);

  // Writes the contents of the given buffer into the file at the given offset.
  // Returns the number of bytes successfully written. This may be less than
  // the buffer size if an error occurs. The file will be extended if the write
  // range spans beyond its length.
  //
  // Important: write only guarantees that the data has been written to the
  // file but it does not guarantee that the data has been persisted to the
  // underlying storage. To ensure that no data loss occurs on system crash,
  // flush must be called and it must successfully return.
  [RaisesException] Promise<unsigned long long> write(
                      [AllowShared]  ArrayBufferView buffer,
                      unsigned long long file_offset);
};
```

Note: read and write currently take a SharedArrayBuffers as a parameter. This is
done to highlight the fact that it might be possible to observe changes to the
buffer as the browser processes it. The implications of this and the possibility
of using simpler ArrayBuffers are being discussed.

##### **Reads and writes**

Open a file, write to it, read from it, and close it to guarantee integrity of
the file’s contents.

```javascript
var file = await nativeIO.open("test_file");  // opens a file (creating it if
                                              // needed)
try {
  var writeSharedArrayBuffer = new SharedArrayBuffer(3);
  var writeBuffer = new Uint8Array(writeSharedArrayBuffer);
  writeBuffer.set([64, 65, 66]);
  await file.write(writeBuffer, 0);  // returns 3, the number of bytes written

  var readSharedArrayBuffer = new SharedArrayBuffer(3);
  var readBuffer = new Uint8Array(readSharedArrayBuffer);
  await file.read(readBuffer, 1);  // Reads at offset 1, returns 2, the number 
                                   // of bytes read

  console.log(readBuffer);  // Uint8Array(3) [65, 66, 0]
} finally {
 file.close();
}
```

##### **List files**

Create/delete a few files and list them.

```javascript
await nativeIO.open("sunrise");
await nativeIO.open("noon");
await nativeIO.open("sunset");
await nativeIO.getAll();  // ["sunset", "sunrise", "noon"]

await nativeIO.delete("noon");
await nativeIO.getAll();  // ["sunrise", "noon"]
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

## Trying It Out

A prototype of Storage Foundation API is available in Chrome Canary. To enable
it, launch Chrome with the `-enable-blink-features=NativeIO `flag or enable
“Experimental Web Platform feature” in ["chrome://flags"](chrome://flags).

To make it easier to try the API, we’ve developed an [Emscripten
Filesystem](https://github.com/fivedots/storage-foundation-emscripten-fs) and a
[tutorial](https://github.com/fivedots/storage-foundation-porting-tutorial) with an
example use-case.  We also provided a
[wrapper](https://github.com/fivedots/storage-foundation-async-wrapper)
that allows directly calling Storage Foundation API from C++ code.

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
