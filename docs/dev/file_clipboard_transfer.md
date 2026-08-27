# File Clipboard Transfer Protocol

Deskflow protocol version 1.9 adds approval-gated transfer of a single regular file copied through the native clipboard. File transfer is enabled only on a TLS-secured Deskflow connection.

## Transfer lifecycle

The server coordinates one file-transfer session at a time between a source screen and the active destination screen.

1. `FCOF%s` — source offers file metadata. No file bytes are sent at this stage.
2. `FCAC%s` — destination explicitly approves the offer.
3. `FCDT%s%s` — source sends a data chunk, currently limited to 64 KiB.
4. `FCRD%s` — destination acknowledges the chunk and allows the next chunk to be sent.
5. `FCEN%s%s` — source declares end of data and supplies the raw 32-byte SHA-256 digest.
6. The destination verifies the declared size and SHA-256 digest, finalises the staged file, and publishes the verified file to its native clipboard.
7. `FCOK%s` — destination confirms that verification and native clipboard publication both succeeded.
8. Only after `FCOK` does the source/server discard the successful transfer session.

`FCCN%s%s` cancels or fails a transfer at any point. In particular, the session remains live after `FCEN`, so a digest-verification or native-clipboard-publication failure at the destination can still be returned to the source instead of being lost.

## State and security invariants

- File data must never be sent before destination approval.
- Only one regular file is supported by the v1.9 MVP; directories, symlinks, and multiple-file offers are rejected.
- The current hard maximum file size is 5 GiB.
- Incoming files are staged with owner-only permissions and are not published to the system clipboard until exact-size and SHA-256 verification succeeds.
- The sender never transmits bytes beyond the size declared in the original offer, including if the source file grows during transfer.
- `FCOK` is accepted only from the session's destination after the server has entered its awaiting-completion state following `FCEN`.
- Once `FCEN` has been sent, a later source clipboard change does not invalidate the already-transmitted file while the destination is verifying it.
- `FCRD` provides per-chunk backpressure so the sender does not stream the entire file without receiver acknowledgement.
- Existing authenticated Deskflow transport is reused; file clipboard transfer does not open another listener or port.

## Primary implementation files

- `src/lib/deskflow/FileTransfer.cpp` — offer and progress serialization/validation.
- `src/lib/deskflow/FileTransferStorage.cpp` — streaming, staging, size enforcement, SHA-256, and cleanup.
- `src/lib/deskflow/ProtocolTypes.cpp` — protocol message codes.
- `src/lib/client/Client.cpp` and `src/lib/client/ServerProxy.cpp` — secondary-side transfer state and protocol I/O.
- `src/lib/server/Server.cpp` and `src/lib/server/ClientProxy1_9.cpp` — routing, approval, backpressure, completion, and cancellation propagation.
- `src/lib/platform/MSWindowsScreen.cpp` and `src/lib/platform/OSXScreen.mm` — native file clipboard integration.
