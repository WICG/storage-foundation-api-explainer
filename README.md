# NativeIO API

## Authors:

* Emanuel Krivoy (krivoy@google.com)
* Jose Lopes (jabolopes@google.com)

## Participate

* Issue Tracker: https://crbug.com/914488
* Discussion forum: https://github.com/fivedots/nativeio-explainer

## Introduction

The NativeIO API is a new performant and low-level storage API that resembles a very basic filesystem. It is isolated by orgin and provides access to file handles. 

It's particularly useful when porting native applications or storage components (e.g. by compiling them to WebAssembly) that expect a filesystem-like interface.

## Goals

We want to allow Developers to “Bring their Own Storage” to the Web by providing a low-level storage API. This new API would be able to function as the backend for varied storage components that have different styles and guarantees, both external to and part of the current web standard.

A few examples of what would be possible with this new API are:
*	Distribute a WASM module for WebSQL, allowing developers to us it across browsers and opening the door to removing the unsupported API from Chrome
*	Allow tried technologies to be performantly used as part of Web applications e.g. using a port of your favorite database within a website
*	Provide a persistent [Emscripten](https://emscripten.org/) filesystem that outperforms [IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs) and has simpler implementation

To fulfill our goals the API must be generic and performant enough to power implementations of existing storage APIs. It should also enable Web developers to implement or port their preferred backend and use it as part of their web application.

## Interface and Examples

There are two main parts to the API:

*	File handles, which provides read and write access to an existing file
*	Filesystem calls, which provides basic functionality to interact with files and file paths

The following are our draft proposals for the interface, written in Web IDL.

### Filesystem Calls

All filesystem calls are accesible through a global NativeIO instance.

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

For this API we opted to have a minimal set of functions. This reduces the surface area of the browser and (hopefully) simplifies the standardization process. More advanced filesystem-like functionality can be implemented by client libraries, as we've done with our prototype Emscripten filesystem.


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

    // Reads the contents of the file into the given buffer, at the given offset
    // until offset + size. Returns the number of bytes read. This may return less
    // bytes than those requested if the read range spans over the end of the file.
    // Returns 0 if the read range is beyond the end of the file. An exception is
    // raised if there is a read failure.
    [RaisesException] unsigned long read(BufferSource buffer, unsigned long long offset, unsigned long size);

    // Writes the given buffer at the given file offset. Returns the number of
    // bytes written. If the write range spans over the end of the file, the
    // file will be resized accordingly. Returns the number of bytes written to
    // the file. The number of bytes written can be less than those requested,
    // for example, because the disk is full. The number of bytes written can
    // never be 0. An exception is raised if there a write failure.
    [RaisesException] unsigned long write(BufferSource buffer, unsigned long long offset);

    // Synchronizes (“flushes”) a file’s in-core state with the storage device.
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
handle = io.openFile("hello-world.txt") // opens a file (creating it if needed)
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
io.createFile("sunrise.jpg")
io.createFile("noon.jpg")
io.createFile("sunset.jpg")

io.listByPrefix("sun") // ["sunset.jpg", "sunrise.jpg"]

// the empty string matches all existing files
io.listByPrefix("") // ["sunset.jpg", "sunrise.jpg", "noon.jpg"]
```

#### Rename and unlink

```javascript
io.createFile("file.txt")
io.rename("file.txt", "newfile.txt")
io.unlink("newfile.txt")
```

## Prototypes

_TODO: Link demos and code once they are easily shareable_

_TODO: Link benchmark results_

In order to discover the functional requirements and validate this API, we decided to port [SQLite](https://www.sqlite.org/index.html) and [LevelDB](https://github.com/google/leveldb) to the web. They were chosen since they are the backends of two existing web APIs, and

In order to run the databases as WebAssembly modules, we decided to compile them with [Emscripten](https://emscripten.org/). This led to us adding a new Emscripten filesystem that uses NativeIO to emulate more complex functionality. Both candidate libraries were succesfully run and showed signnificant performance improvements when compared to the same libraries running on top of the [Chrome filesystem](https://developer.mozilla.org/en-US/docs/Web/API/Window/requestFileSystem).

## Security Considerations

The API will have similar security policies to the one used in modern web storage APIs. Access to files will be isolated per origin. Quota checks will be used to prevent misuse of user resources.
