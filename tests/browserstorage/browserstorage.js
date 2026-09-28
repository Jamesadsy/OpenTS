(function () {
  "use strict";

  const DATA_ROOT = "/data";
  const USER_ROOT = "/userdata";
  const BLOCK_BYTES = 256 * 1024;
  const CACHE_LIMIT_BYTES = 16 * 1024 * 1024;
  const GENERATION_PATH = "OpenTS/124H/generations/synthetic-v1";
  const TS_GAME = "ts";
  const DATA_FILES = [
    { path: "ROOT.TST", size: 97, seed: 11 },
    { path: "NESTED/SAMPLE.BIN", size: 513, seed: 37 },
    { path: "TIBSUN.MIX", size: 257, seed: 73, sentinel: true }
  ];
  const USER_FILE = { name: "124h-synthetic-user.bin", size: 257, seed: 91 };

  function bytePattern(seed, index) {
    return (seed + index * 17 + (index >> 3) * 29) & 255;
  }

  function userBytePattern(index) {
    return (USER_FILE.seed + index * 29 + (index >> 4) * 13) & 255;
  }

  function makeDataBytes(spec) {
    const bytes = new Uint8Array(spec.size);
    const marker = spec.sentinel
      ? new TextEncoder().encode("VV124H-SYNTHETIC-ONLY:")
      : new Uint8Array(0);
    bytes.set(marker.subarray(0, bytes.length));
    for (let i = marker.length; i < bytes.length; i += 1) bytes[i] = bytePattern(spec.seed, i);
    return bytes;
  }

  function makeUserBytes() {
    return Uint8Array.from({ length: USER_FILE.size }, (_, index) => userBytePattern(index));
  }

  function toHex(bytes) {
    let output = "";
    for (const byte of bytes) output += byte.toString(16).padStart(2, "0");
    return output;
  }

  async function sha256(bytes) {
    const digest = await crypto.subtle.digest("SHA-256", bytes);
    return toHex(new Uint8Array(digest));
  }

  async function writeOpfsFile(root, relativePath, bytes) {
    const parts = relativePath.split("/");
    const fileName = parts.pop();
    let directory = root;
    for (const part of parts) directory = await directory.getDirectoryHandle(part, { create: true });
    const handle = await directory.getFileHandle(fileName, { create: true });
    const writer = await handle.createWritable({ keepExistingData: false });
    await writer.write(bytes);
    await writer.close();
    return handle;
  }

  async function publishSyntheticGeneration() {
    if (!navigator.storage || typeof navigator.storage.getDirectory !== "function") {
      throw new Error("OPFS is unavailable; browser Data storage cannot be prepared.");
    }
    const opfsRoot = await navigator.storage.getDirectory();
    const openTSRoot = await opfsRoot.getDirectoryHandle("OpenTS", { create: true });
    const proofRoot = await openTSRoot.getDirectoryHandle("124H", { create: true });
    const generations = await proofRoot.getDirectoryHandle("generations", { create: true });
    const generation = await generations.getDirectoryHandle("synthetic-v1", { create: true });
    const entries = [];
    let totalBytes = 0;

    for (const spec of DATA_FILES) {
      const expected = makeDataBytes(spec);
      const handle = await writeOpfsFile(generation, spec.path, expected);
      const file = await handle.getFile();
      const actual = new Uint8Array(await file.arrayBuffer());
      const digest = await sha256(actual);
      if (actual.length !== spec.size || !actual.every((value, index) => value === expected[index])) {
        throw new Error("Synthetic OPFS fixture did not verify: " + spec.path);
      }
      entries.push({ path: spec.path, file, size: file.size, sha256: digest, synthetic: true });
      totalBytes += file.size;
    }

    const manifest = {
      generation: "synthetic-v1",
      publishedAt: new Date().toISOString(),
      syntheticOnly: true,
      files: entries.map(({ path, size, sha256: hash }) => ({ path, size, sha256: hash }))
    };
    const marker = await proofRoot.getFileHandle("active-data-generation.json", { create: true });
    const markerWriter = await marker.createWritable({ keepExistingData: false });
    await markerWriter.write(JSON.stringify(manifest));
    await markerWriter.close();
    const markerText = await (await marker.getFile()).text();
    const published = JSON.parse(markerText);
    if (published.generation !== "synthetic-v1" || published.files.length !== DATA_FILES.length) {
      throw new Error("Synthetic OPFS generation publication marker did not verify.");
    }

    return {
      generationPath: "OPFS:/" + GENERATION_PATH,
      engineDataRoot: DATA_ROOT,
      publication: "PASS",
      syntheticOnly: true,
      fileCount: entries.length,
      totalBytes,
      files: manifest.files,
      entries
    };
  }

  function readLocalSlice(blob) {
    if (blob.size > BLOCK_BYTES) throw new Error("Data read exceeded the bounded 256 KiB local slice.");
    const url = URL.createObjectURL(blob);
    try {
      const xhr = new XMLHttpRequest();
      xhr.open("GET", url, false);
      xhr.overrideMimeType("text/plain; charset=x-user-defined");
      xhr.send();
      if (xhr.status !== 200 || xhr.responseText.length !== blob.size) {
        throw new Error("Browser-local OPFS slice read failed.");
      }
      return Uint8Array.from(xhr.responseText, character => character.charCodeAt(0) & 255);
    } finally {
      URL.revokeObjectURL(url);
    }
  }

  function probeLocalReader() {
    const expected = Uint8Array.from({ length: 1024 }, (_, index) => index & 255);
    const actual = readLocalSlice(new Blob([expected]));
    if (!expected.every((value, index) => value === actual[index])) {
      throw new Error("Browser cannot read local binary slices without corruption.");
    }
  }

  function mountReadOnlyData(FS, entries, mountpoint) {
    probeLocalReader();
    const cache = new Map();
    let cacheBytes = 0;
    let readBlocks = 0;

    function block(entry, index) {
      const key = entry.path + ":" + index;
      if (cache.has(key)) {
        const value = cache.get(key);
        cache.delete(key);
        cache.set(key, value);
        return value;
      }
      const start = index * BLOCK_BYTES;
      const value = readLocalSlice(entry.file.slice(start, Math.min(start + BLOCK_BYTES, entry.size)));
      readBlocks += 1;
      while (cacheBytes + value.length > CACHE_LIMIT_BYTES && cache.size > 0) {
        const oldest = cache.keys().next().value;
        cacheBytes -= cache.get(oldest).length;
        cache.delete(oldest);
      }
      cache.set(key, value);
      cacheBytes += value.length;
      return value;
    }

    const fail = code => { throw new FS.ErrnoError(code); };
    const directoryStreamOps = {
      llseek(stream, offset, whence) {
        const position = offset + (whence === 1 ? stream.position : 0);
        if (position < 0) fail(28);
        return position;
      }
    };
    const nodeOps = {
      getattr(node) {
        const size = node.file ? node.file.size : 4096;
        return {
          dev: 1, ino: node.id, mode: node.mode, nlink: 1, uid: 0, gid: 0, rdev: 0,
          size, atime: new Date(0), mtime: new Date(0), ctime: new Date(0),
          blksize: 4096, blocks: Math.ceil(size / 4096)
        };
      },
      setattr() { fail(69); },
      lookup(parent, name) { return parent.contents[name.toUpperCase()] || fail(44); },
      readdir(node) { return [".", "..", ...Object.keys(node.contents)]; },
      mknod() { fail(69); },
      rename() { fail(69); },
      unlink() { fail(69); },
      rmdir() { fail(69); },
      symlink() { fail(69); }
    };
    const streamOps = {
      open(stream) { if ((stream.flags & 3) !== 0) fail(69); },
      read(stream, buffer, offset, length, position) {
        if (position < 0) fail(28);
        const entry = stream.node.file;
        if (!entry) fail(31);
        const count = Math.min(length, Math.max(0, entry.size - position));
        let copied = 0;
        while (copied < count) {
          const at = position + copied;
          const bytes = block(entry, Math.floor(at / BLOCK_BYTES));
          const within = at % BLOCK_BYTES;
          const amount = Math.min(bytes.length - within, count - copied);
          buffer.set(bytes.subarray(within, within + amount), offset + copied);
          copied += amount;
        }
        return copied;
      },
      llseek(stream, offset, whence) {
        const file = stream.node.file;
        const position = offset + (whence === 1 ? stream.position : whence === 2 ? file.size : 0);
        if (position < 0) fail(28);
        return position;
      },
      write() { fail(69); },
      mmap() { fail(43); }
    };
    const backend = {
      mount() {
        const root = FS.createNode(null, "/", 0o40555, 0);
        root.node_ops = nodeOps;
        root.stream_ops = directoryStreamOps;
        root.contents = Object.create(null);
        for (const entry of entries) {
          let parent = root;
          const parts = entry.path.split("/");
          const name = parts.pop();
          for (const part of parts) {
            const key = part.toUpperCase();
            if (!parent.contents[key]) {
              const directory = FS.createNode(parent, part, 0o40555, 0);
              directory.node_ops = nodeOps;
              directory.stream_ops = directoryStreamOps;
              directory.contents = Object.create(null);
              parent.contents[key] = directory;
            }
            parent = parent.contents[key];
          }
          const file = FS.createNode(parent, name, 0o100444, 0);
          file.file = entry;
          file.node_ops = nodeOps;
          file.stream_ops = streamOps;
          parent.contents[name.toUpperCase()] = file;
        }
        return root;
      }
    };

    FS.mkdirTree(mountpoint);
    FS.mount(backend, {}, mountpoint);
    return {
      stats() { return { readBlockBytes: BLOCK_BYTES, cacheLimitBytes: CACHE_LIMIT_BYTES, cacheBytes, readBlocks }; },
      clear() { cache.clear(); cacheBytes = 0; }
    };
  }

  function mountUserRoot(FS) {
    const idbfs = FS.filesystems && FS.filesystems.IDBFS;
    if (!idbfs) throw new Error("IDBFS is unavailable; writable User storage cannot be mounted.");
    FS.mkdir(USER_ROOT);
    FS.mount(idbfs, {}, USER_ROOT);
  }

  async function prepareImportedOwnerData(Module) {
    if (!navigator.storage || typeof navigator.storage.getDirectory !== "function") {
      throw new Error("OPFS is unavailable; imported TS data cannot be mounted.");
    }
    const [deviceStorage, profile] = await Promise.all([
      import("/device/storage.mjs"),
      import("/device/profile.mjs")
    ]);
    const scope = deviceStorage.localInstallationScope();
    const installed = await deviceStorage.installedFiles(scope, undefined, TS_GAME);
    if (!installed) throw new Error("Import the accepted TS .vcvault on this device before opening OpenTS.");
    const readiness = profile.profileReadiness(TS_GAME, installed.state.files);
    if (!readiness.ready) throw new Error("The installed TS profile is incomplete.");

    const entries = installed.files.map(entry => ({
      path: entry.path,
      file: entry.file,
      size: entry.sizeBytes
    }));
    const mount = mountReadOnlyData(Module.FS, entries, DATA_ROOT);
    const totalBytes = entries.reduce((total, entry) => total + entry.size, 0);
    return {
      generation: {
        game: TS_GAME,
        generation: installed.state.generation,
        fileCount: entries.length,
        totalBytes,
        contentSource: "the installed Vanilla Vault TS OPFS generation",
        copiedOwnerData: false
      },
      mount
    };
  }

  function syncfs(FS, populate) {
    return new Promise((resolve, reject) => {
      FS.syncfs(populate, error => error ? reject(error) : resolve());
    });
  }

  window.OpenTS124HStorage = {
    DATA_ROOT,
    USER_ROOT,
    BLOCK_BYTES,
    CACHE_LIMIT_BYTES,
    USER_FILE,
    async prepareData(Module) {
      const generation = await publishSyntheticGeneration();
      const mount = mountReadOnlyData(Module.FS, generation.entries, DATA_ROOT);
      return { generation: { ...generation, entries: undefined }, mount };
    },
    prepareImportedOwnerData,
    mountUserRoot(Module) { mountUserRoot(Module.FS); },
    hydrateUser(Module, injectFailure) {
      if (injectFailure) {
        return new Promise((_, reject) => setTimeout(
          () => reject(new Error("Injected synthetic initial IDBFS hydration failure.")), 0));
      }
      return syncfs(Module.FS, true);
    },
    flushUser(Module) { return syncfs(Module.FS, false); },
    expectedUserBytes: makeUserBytes,
    bytesToHex: toHex,
    sha256,
    remoteDataRequests() {
      return performance.getEntriesByType("resource")
        .filter(entry => /(?:\.mix(?:$|\?)|\/data\/|owner[-_]?data)/i.test(entry.name))
        .map(entry => entry.name);
    }
  };
}());
