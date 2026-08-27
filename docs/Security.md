# Security Policy

## Supported Versions

The latest minor release is supported and receives security updates:
https://github.com/deskflow/deskflow/releases

## Reporting a Vulnerability

Please report vulnerabilities on our issue tracker as bugs:
https://github.com/deskflow/deskflow/issues

## File Clipboard Transfers

File clipboard transfers require TLS and reuse Deskflow's existing authenticated connection and configured TCP port;
the feature does not add a listener or create firewall exceptions. The destination must approve an offer before any
file content is sent. Received content is written through normal operating-system file APIs to a private staging
directory, without antivirus exclusions, and is published to the native clipboard only after its declared size and
SHA-256 digest have been verified.

Malformed offers are rejected before staging. This includes invalid transfer IDs, empty or unsafe filenames,
directories, and files larger than the configured implementation limit.

The first implementation accepts one regular file up to 5 GiB. Directories, symbolic links, multiple selections, and
unsafe cross-platform filenames are rejected.
