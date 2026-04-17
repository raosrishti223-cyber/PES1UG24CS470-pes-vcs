
## Building PES-VCS — A Version Control System from Scratch

**Objective:** Build a local version control system that tracks file changes, stores snapshots efficiently, and supports commit history. Every component maps directly to operating system and filesystem concepts.

**Platform:** Ubuntu 22.04

---



This repository implements a simplified version control system similar to Git. The following core components have been modified:

* `object.c` → Content-addressable object storage system (blob/tree/commit storage)
* `tree.c` → Directory snapshot construction from staging area
* `index.c` → Staging area (index) implementation
* `commit.c` → Commit creation and history tracking

---

## Build Instructions

### Install Dependencies

```bash
sudo apt update && sudo apt install -y gcc build-essential libssl-dev
```

---

### Build Project

```bash
make clean
make
```

---

## Repository Usage

### Initialize Repository

```bash
./pes init
```

This creates a `.pes/` directory containing:

* object storage (`.pes/objects`)
* references (`.pes/refs`)
* HEAD pointer

---

### Add Files to Staging Area

```bash
./pes add <file>
```

Example:

```bash
./pes add a.txt b.txt
```

---

### Check Status

```bash
./pes status
```

Shows:

* staged files
* modified files
* deleted files
* untracked files

---

### Create Commit

```bash
./pes commit -m "message"
```

This performs:

1. Builds a tree from the staging index (`tree_from_index`)
2. Reads parent commit from HEAD
3. Creates a commit object
4. Stores commit in object database
5. Updates HEAD reference

---

### View Commit History

```bash
./pes log
```

Displays commit chain from latest to oldest.

---

## Internal Implementation Overview

### 1. Object Storage (`object.c`)

Implements content-addressable storage:

* Each object is stored as:

  ```
  <type> <size>\0<data>
  ```
* Objects are identified using SHA-256 hash
* Stored in sharded directories:

  ```
  .pes/objects/xx/yyyy...
  ```
* Supports:

  * `object_write()` → stores blob/tree/commit
  * `object_read()` → retrieves and verifies integrity

---

### 2. Tree Construction (`tree.c`)

* Converts index entries into hierarchical directory trees
* Handles recursive directory structure
* Formats entries as:

  ```
  <mode> <name>\0<32-byte-hash>
  ```
* Writes tree objects using `object_write()`

---

### 3. Index (Staging Area) (`index.c`)

* Stores staged file metadata in text format:

  ```
  <mode> <hash> <mtime> <size> <path>
  ```
* Supports:

  * `index_load()` → loads index from disk
  * `index_save()` → writes index atomically
  * `index_add()` → stages files as blobs

---

### 4. Commit System (`commit.c`)

* Creates commit objects from staged state
* Stores:

  * tree hash
  * parent commit
  * author info
  * timestamp
  * commit message
* Updates HEAD after commit

---

## Object Model

### Blob

Stores file content.

```
blob <size>\0<data>
```

---

### Tree

Represents directory structure.

```
100644 file.txt\0<hash>
040000 dir\0<hash>
```

---

### Commit

Represents snapshot of repository.

```
tree <hash>
parent <hash>

author <name> <timestamp>
committer <name> <timestamp>

message
```

---

## Workflow Summary

```
Working Directory
        ↓
   pes add
        ↓
     Index
        ↓
   pes commit
        ↓
   Tree Object
        ↓
   Commit Object
        ↓
   HEAD updated
```

---

## Testing

### Build and run tests

```bash
make test_objects
make test_tree
make test-integration
```

---

### Manual test flow

```bash
./pes init
echo "hello" > a.txt
./pes add a.txt
./pes commit -m "first commit"
./pes log
```

---

## Key Concepts Implemented

* Content-addressable storage (SHA-256 based)
* Directory tree construction
* Staging area (index) design
* Commit graph using parent pointers
* Atomic file operations
* Snapshot-based version control

---



