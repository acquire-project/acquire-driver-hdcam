# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.7](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.6...v0.1.7) - 2023-10-02

### Fixes

- Driver builds as an OSX universal binary.

## [0.1.6](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.5...v0.1.6) - 2023-08-15

- Fixes a bug where the output trigger source property was set when it shouldn't be.
- Fixes a bug where roi bound checking wasn't working.

## [0.1.5](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.4...v0.1.5) - 2023-07-28

- Updates to latest acquire-core-libs.

## [0.1.4](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.3...v0.1.4) - 2023-06-27

### Changes

- Reflect a change in core-libs: `StorageProperties::chunking::max_bytes_per_chunk` is now a `uint64_t` (was
  a `uint32_t`).

## [0.1.3](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.2...v0.1.3) - 2023-06-02

### Added

- Nightly releases.

### Changes

- Updates call to `storage_properties_init` for the new function signature.

## [0.1.2](https://github.com/acquire-project/acquire-driver-hdcam/compare/v0.1.1...v0.1.2) - 2023-05-12

### Fixed

- Fixes a bug where `acquire_abort` hangs for a max_frame_count of anything other than 0.

## 0.1.1 - 2023-05-11
