# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **macOS sandbox environment** for running GPU-accelerated applications with security isolation. It provides near-native Metal/GPU performance (98-99%) while maintaining file system and network restrictions similar to container environments. The security model mimics a devcontainer-like setup with container-style isolation.

## Common Commands

### Running Sandboxed Programs

```bash
# Simple usage
./sandbox.sh -- ./your-program [args...]

# Disable network access (for untrusted code)
./sandbox.sh -n -- ./untrusted-program

# Specify different workspace
./sandbox.sh -d /path/to/workspace -- ./program

# Verbose mode (show sandbox profile)
./sandbox.sh -v -- ./program

# Allow additional read-only directories
./sandbox.sh -r /usr/local/data -- ./data-processor

# Allow additional writable directories
./sandbox.sh -w /tmp/output -- ./program

# Multiple options combined
./sandbox.sh -n -v -r /data -- ./program
```

### Testing

```bash
# Compile test program
gcc test_sandbox.c -o test_sandbox

# Run tests to verify sandbox restrictions
./sandbox.sh -- ./test_sandbox

# Expected results:
# ✓ Can write to workspace
# ✓ Can write to /tmp
# ✓ CANNOT write to home directory
# ✓ Can read system files
# ✓ Child processes inherit restrictions
```

## Architecture

### Core Components

1. **sandbox.sh** - Main sandbox script
   - Simple by default, advanced when needed
   - Supports all options: workspace, network, additional directories
   - Comprehensive inline documentation
   - ~400 lines with detailed comments

2. **test_sandbox.c** - Verification test program
   - Tests file write permissions across different locations
   - Verifies child process inheritance
   - Confirms sandbox restrictions work as expected

### How It Works

The script uses **Apple's TrustedBSD Mandatory Access Control (MAC)** framework via the `sandbox-exec` command. It generates a sandbox profile (in Scheme-like syntax) that defines:

1. **Default deny policy** - Start with no permissions
2. **Process operations** - Allow fork, exec, signals
3. **Metal/GPU access** - Full IOKit permissions for native performance
4. **File system rules**:
   - Read: Everything (system files, home directory)
   - Write: Only workspace + /tmp
   - Deny: Sensitive locations (~/.ssh, ~/.aws, cloud credentials, shell configs)
5. **Network rules** - Configurable (enabled by default, can disable with `-n`)

### Security Model (Container-Like Isolation)

The sandbox mimics these container security features:

| Container Feature | macOS Equivalent | Implementation |
|------------------|------------------|----------------|
| `--cap-drop=ALL` | Default deny policy | `(deny default)` |
| `--cap-add=SETUID/SETGID` | Process operations | `(allow process-exec* process-fork)` |
| Mount restrictions | File system rules | `(allow file-write* (subpath "$WORKDIR"))` |
| `--network=slirp4netns` | Network isolation | `(allow/deny network*)` |
| `--security-opt=no-new-privileges` | Write denials | Block ~/.ssh, ~/.aws, shell configs |
| Metal/GPU access | IOKit permissions | `(allow iokit-open ...)` |

**Key Limitation**: Unlike containers, the sandbox runs as the current user. For stronger isolation, use a dedicated macOS user account (see README.md for setup instructions).

## Key Implementation Details

### Sandbox Profile Generation

The script:
1. Creates a temporary profile file using Scheme syntax
2. Replaces placeholders with actual paths (workspace, home directory)
3. Adds user-specified rules (additional dirs, network settings)
4. Passes profile to `sandbox-exec -f $PROFILE_FILE command`
5. Cleans up profile file on exit

### Critical Paths

- **sandbox.sh:136-270** - Profile template with placeholder replacement
- **sandbox.sh:310-365** - Dynamic rule generation for custom directories
- **sandbox.sh:180-200** - Metal/GPU IOKit permissions
- **test_sandbox.c:20-31** - Home directory write test (should fail)

### Metal/GPU Performance

The script grants access to:
- Metal Compiler Service (via mach-lookup)
- IOKit accelerator classes (IOAccelerator, IOSurfaceRootUserClient)
- Core Video services (for GPU rendering)

This ensures 98-99% of bare metal performance (vs. 10-30% in VMs).

## File System Isolation Details

### Readable Everywhere
- `/System`, `/Library`, `/Applications`, `/usr`, `/bin`, `/sbin`
- User home directory (read-only)
- Standard device files (`/dev/null`, `/dev/random`, etc.)

### Writable Only In
- Workspace directory (specified with `-d`, defaults to current directory)
- `/tmp`, `/var/tmp`, `/private/tmp`, `/private/var/tmp`
- Additional directories specified with `-w` flag

### Explicitly Blocked (Write)
- `~/.ssh` - SSH keys
- `~/.aws` - AWS credentials
- `~/.config/gcloud` - GCP credentials
- `~/.azure` - Azure credentials
- `~/.kube` - Kubernetes config
- `~/.bashrc`, `~/.zshrc`, `~/.bash_profile`, `~/.profile` - Shell configs

## Common Patterns

### Using with Stronger Isolation

Create a dedicated user for untrusted code:

```bash
# Create sandbox user (one-time setup)
sudo dscl . -create /Users/sandbox-dev
sudo dscl . -create /Users/sandbox-dev UserShell /bin/bash
sudo dscl . -create /Users/sandbox-dev UniqueID 5000
sudo dscl . -create /Users/sandbox-dev PrimaryGroupID 20
sudo dscl . -create /Users/sandbox-dev NFSHomeDirectory /Users/sandbox-dev
sudo dscl . -passwd /Users/sandbox-dev
sudo mkdir -p /Users/sandbox-dev
sudo chown sandbox-dev:staff /Users/sandbox-dev

# Run as sandbox user
sudo -u sandbox-dev ./sandbox.sh -- ./untrusted-program
```

Benefits: Separate keychain, no access to personal files, mimics container user model.

### Debugging Sandbox Issues

```bash
# See exact sandbox profile
./sandbox.sh -v -- ./program

# Test with simple command first
./sandbox.sh -- /bin/ls

# Check if program needs write access
./sandbox.sh -- /bin/bash -c "whoami && pwd && ls -la"
```

### Integration with Build Systems

```bash
# Run tests in sandbox
./sandbox.sh -- make test

# Build in isolated environment
./sandbox.sh -d $PWD/build -- cmake .. && make

# Run with network disabled
./sandbox.sh -n -- ./run-offline-tests.sh
```

## Usage Flexibility

**sandbox.sh** provides both simple and advanced usage:
- Default behavior: workspace isolation, network enabled
- Simple options: `-n` (no network), `-v` (verbose), `-d DIR` (workspace)
- Advanced options: `-r DIR` (read-only dirs), `-w DIR` (writable dirs)
- All options can be combined as needed
- Template-based profile with placeholder replacement
- Extensive comments explaining security model

## Related Files

- **README.md** - Complete documentation, security comparison, usage examples, troubleshooting
