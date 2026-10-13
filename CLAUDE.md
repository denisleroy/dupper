# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`dupper` is a small C++11 command-line tool that finds duplicate files under a directory tree. Usage: `./dupper [-v] [-m <minsize>] [--exclude-dir <name>]... [--keep-shortest [-n]] <path>`.

## Build

```sh
make          # builds ./dupper (CXXFLAGS = -g -O0 -std=c++11)
make clean    # removes the binary and .o files
```

Every `.o` depends on all headers (`$(wildcard *.h)`), so a header edit rebuilds everything. New `.cc` files must be added to `SOURCES` in the Makefile. There is no test suite, linter, or `.gitignore`; build outputs (`*.o`, `dupper`) sit untracked in the repo root, so don't commit them.

## Architecture

The program narrows sets of files through successive comparison passes:

1. **Scan** (`scanner.cc`): `FileScanner::scan` walks the tree recursively with `opendir`/`readdir` and collects regular files (`DT_REG`) into a flat `vector<File>`. Symlinks and other types are skipped, and so are subdirectories whose name is in `Context::exclude_dirs` (the root `path` itself is never excluded). While it scans, it prints a progress line using ANSI escapes and the terminal width from `TIOCGWINSZ`.
2. **Comparator pipeline** (`main.cc`): an `FTable` (`vector<vector<File>>`) holds groups of candidate duplicates. It starts as a single group of all files. `apply_comparator` runs a `Comparator` over each group and returns a new `FTable` of refined groups. The passes run in this order:
   - `SizeComparator`: `stat`s each file, fills in `File::metadata`, and groups files by `st_size`. Zero-size files and files smaller than `minsize` are dropped.
   - `Md5Comparator(ctx, 0, 512)`: groups files by the MD5 of their first 512 bytes.
   - `Md5Comparator(ctx, 0, 0)`: groups files by the MD5 of the whole file (`maxsize == 0` means the entire file). This last pass's output is what gets printed.

   After every pass, groups with only one file are discarded.

   With `--keep-shortest`, `keep_shortest()` then goes over the final groups. When every file in a group is in the same directory, it keeps the file with the shortest name and `unlink`s the rest. Groups that span more than one directory are left alone. With `-n`, nothing is deleted. Instead the shell-quoted `rm` commands are written to `./DELETE.sh`, which is overwritten on each run.
3. **Comparator interface** (`comparator.h`): `run(files, candidates, duplicates)`. `candidates` receives groups that still need checking. `duplicates` is meant for groups that are already confirmed duplicates; right now only `SizeComparator`'s zero-size branch writes to it, and that branch can't be reached. To add a new pass, subclass `Comparator` and slot it into the sequence in `main()`.
4. **MD5** (`md5.cc`/`md5.h`): this is the RSA reference implementation. `MDFile` was changed to `mmap` a byte range (`map_offset`, `map_len`) instead of reading the file in a loop. The digest is stored as a `std::vector<unsigned char>` so it can be used directly as a `std::map` key.
5. **Context** (`context.cc`/`context.h`): parses options with `getopt_long` and holds the run-wide settings (`verbose`, `minsize`, `ignore_empty_files`, `keep_shortest`, `dry_run`, `exclude_dirs`, `path`). Every comparator, and `FileScanner`, gets a reference to it.

## Known rough edges

These are things to be aware of, not things to fix unless asked:
- `ignore_empty_files` can't be set from the command line.
- `--scan` only prints a message.
- `main.cc` allocates `FTable`s with `new` but releases them with `free()`.
