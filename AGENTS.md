# AGENTS.md

This file provides guidance for AI agents and developers working with
the `git-for-windows/7-Zip` repository.

## Repository Purpose

This repository is a fork of [Igor Pavlov's 7-Zip](https://www.7-zip.org/)
that carries a small patch series adding enhanced SFX (self-extracting
archive) functionality needed by Git for Windows.

Git for Windows distributes two products as self-extracting `.7z`
archives rather than conventional installers:

- **Portable Git** ("USB drive edition") -- a self-contained Git
  installation that does not require running an installer. The user
  double-clicks the `.7z.exe`, picks an extraction directory, and the
  SFX module extracts the archive there and runs a post-install script.
- **Git for Windows SDK** -- the full development environment for
  building Git for Windows. Also distributed as a self-extracting
  archive, same mechanism.

The official 7-Zip SFX module only supports extracting to a temporary
directory and then running an embedded `setup.exe`, which does not fit
these use cases. This fork adds SFX configuration directives such as
`ExtractPathText`, `InstallPath`, `CancelPrompt`, and
`ExtractDialogText` that allow the self-extracting archive to ask the
user for a permanent extraction path and to display customized prompts
during extraction. The release scripts in `build-extra` concatenate the
SFX module, a configuration block, and the `.7z` payload into the final
`.7z.exe`:

```
cat 7zS.sfx config.txt payload.7z > PortableGit.7z.exe
```

The SFX configuration block uses 7-Zip's `;!@Install@!UTF-8!` /
`;!@InstallEnd@!` delimiters and the directives implemented by this
fork.

### Historical Context

Since its inception in 2007, Git for Windows relied on the "modified
SFX" from 7zsfx.info, a third-party project that extended the official
7-Zip SFX with features like user-selectable extraction paths and cancel
prompts. That project went defunct around 2016. This fork reimplements
the features Git for Windows actually uses directly on top of the
official 7-Zip source, ensuring the SFX module stays current with
upstream security and compression improvements.

## Relationship to Other Repositories

This repository is part of a larger ecosystem of repositories that make
up the Git for Windows project:

- **[git-for-windows/build-extra](https://github.com/git-for-windows/build-extra)**
  stores the compiled SFX binaries (`7-Zip/7zS.sfx` and
  `7-Zip/7zSD.sfx`) and contains the release scripts (`portable/release.sh`,
  `sdk-installer/release.sh`) that assemble them into self-extracting
  archives.
- **[msys2/MINGW-packages](https://github.com/msys2/MINGW-packages)**
  contains the MSYS2 package definition for 7-zip (`mingw-w64-7zip/PKGBUILD`),
  which is the standard `7z` command-line tool used inside the Git for
  Windows SDK and during builds. This is separate from the SFX module
  but tracks the same upstream releases.
- **[git-for-windows/git-for-windows-automation](https://github.com/git-for-windows/git-for-windows-automation)**
  contains the CI/CD workflows that build release artifacts; those
  workflows depend on the SFX binaries in `build-extra`.

When a new upstream 7-Zip version is released, two independent update
tracks run:

- **SFX track** (sequential): update this repository first, wait for
  CI to produce artifacts, then update `build-extra` with the new
  binaries.
- **MSYS2 track** (independent): update the `mingw-w64-7zip` PKGBUILD
  in `msys2/MINGW-packages`.

## Branch Model

The repository uses a rebase-based workflow with two kinds of branches:

- **`upstream`**: A linear history of vanilla 7-Zip source imports. Each
  commit replaces the entire tree with the contents of a new source
  archive (e.g. `7z2601-src.7z`). No patches are applied here.

- **`v<VERSION>-VS2022-sfx`** (e.g. `v26.01-VS2022-sfx`): The active
  development branch. It contains the `upstream` commit for that version
  at its root, with the Git for Windows patch series rebased on top.
  When a new upstream version is released, a new branch is created by
  rebasing the patches from the previous version branch onto the new
  upstream commit.

The patch series uses merge commits to organize topic branches:

```
* Merge pull request #30 (dependabot: upload-artifact-7)
* Merge pull request #28 (dependabot: checkout-6)
* Merge branch 'readme' (README.md, Dependabot config)
* Merge pull request #8 (github-workflow: CI build definition)
* Merge branch 'sfxsetup-gfw-extra' (SFX enhancements: -o, CancelPrompt, etc.)
* Merge branch 'sfxsetup-vs2022' (VS2022 project conversion)
* Merge branch 'fm-vs2022' (File Manager VS2022 conversion)
* Merge branch 'gitignore' (.gitignore, .gitattributes)
```

Because of this merge structure, rebasing onto a new upstream version
must always use `git rebase --rebase-merges` to preserve the topology.

The default branch on GitHub should always point to the latest
`v*-VS2022-sfx` branch. It is updated after the corresponding
`build-extra` PR is merged.

## Build System

The SFX module is built with MSBuild (Visual Studio 2022) from the
solution file at `CPP/7zip/Bundles/SFXSetup/SFXSetup.sln`. Two
configurations produce the two SFX variants:

| Configuration | Output    | Linking                           |
|---------------|-----------|-----------------------------------|
| `Release`     | `7zS.sfx` | Statically linked (no DLL deps)   |
| `ReleaseD`    | `7zSD.sfx`| Dynamically linked (`vcruntime140.dll`) |

`7zS.sfx` is used in practice by the Portable Git and SDK installer
release scripts. `7zSD.sfx` is built and shipped as well but is not
currently used by any release script.

## CI

The GitHub Actions workflow `.github/workflows/msbuild.yml` runs on
every push and pull request. It builds both configurations in a matrix
and uploads the resulting `.sfx` files as artifacts named `7zS` and
`7zSD`. The CI run typically completes in 1-2 minutes.

After pushing a new version branch, the CI artifacts are downloaded and
committed into `build-extra`.

## Directory Structure

```
.github/
  workflows/
    msbuild.yml          # CI: builds SFXSetup for Release and ReleaseD
  dependabot.yml         # Keeps GitHub Actions dependencies up to date

AGENTS.md                # This file
README.md                # Project overview and history

Asm/                     # 7-Zip assembly sources (upstream)
C/                       # 7-Zip C sources (upstream)
CPP/                     # 7-Zip C++ sources (upstream + patches)
  7zip/
    Bundles/
      SFXSetup/          # The SFX module built by this project
        SFXSetup.sln     # MSBuild solution file
        SfxSetup.cpp     # Main SFX entry point (patched)
        ...
DOC/                     # 7-Zip documentation (upstream)
```

The patches primarily modify files under `CPP/7zip/Bundles/SFXSetup/`.

---

# 7-Zip Update Agent

You are an agent that updates the 7-Zip SFX component used by Git for
Windows whenever a new upstream 7-Zip version is released. Two update
tracks run in parallel:

- **SFX track** (sequential): `git-for-windows/7-Zip`, then
  `git-for-windows/build-extra` (depends on 7-Zip CI artifacts).
- **MSYS2 track** (independent): `msys2/MINGW-packages`.

## Repositories

1. **git-for-windows/7-Zip** -- A fork of 7-Zip with SFX enhancements.
   Has an `upstream` branch (vanilla source imports) and version
   branches like `v25.01-VS2022-sfx` (patches rebased on upstream).
2. **git-for-windows/build-extra** -- Stores compiled `7zS.sfx` and
   `7zSD.sfx` binaries in its `7-Zip/` directory.
3. **msys2/MINGW-packages** -- MSYS2 package definition for 7-zip in
   `mingw-w64-7zip/PKGBUILD`. The user's fork is `dscho/MINGW-packages`.

## Release Notes

Release notes are posted as discussion threads in the SourceForge
forum at `https://sourceforge.net/p/sevenzip/discussion/45797/`.
Each version gets a thread titled "7-Zip XX.YY" by Igor Pavlov.
The RSS feed at `https://sourceforge.net/p/sevenzip/discussion/45797/feed.rss`
can be used for programmatic discovery. The thread URL pattern is
`/p/sevenzip/discussion/45797/thread/<thread-id>/`.

These release notes are included in the build-extra PR body when
available.

## Working Directory

All clones go under the current working directory (typically
`g4w-automation/`). A `7-Zip/` clone and `build-extra/` clone will
be created or reused if they already exist.

## Step-by-Step Procedure

### Step 1: Determine the new version

The user will tell you a new 7-Zip version is available. Determine
the version number (e.g. `26.00`) and derive `<VVER>` (version with
dots removed, e.g. `2600`).

### Step 2: Pre-flight checks

1. Check if `git-for-windows/7-Zip` already has a branch named
   `v<VERSION>-VS2022-sfx`. If it does, skip to Step 5.
2. Check if `git-for-windows/build-extra` already has a PR or branch
   for this version. If a merged PR exists, skip to Step 7.
3. Check if `msys2/MINGW-packages` already has a merged PR for this
   version (search for `7zip: Update to <VERSION>` or similar).

### Step 3: Import new upstream source

1. Clone `git-for-windows/7-Zip`. For simplicity, do a full clone.
2. Identify the current latest `v*-VS2022-sfx` branch by parsing
   remote branch names. The default branch is NOT always the newest.
3. Check out the `upstream` branch.
4. Download the source archive from SourceForge. **Use the `.7z`
   archive** (not `.tar.xz`), matching the established import
   commit messages:
   ```
   curl -L -o /tmp/7z<VVER>-src.7z \
     'https://sourceforge.net/projects/sevenzip/files/7-Zip/<VERSION>/7z<VVER>-src.7z/download'
   ```
5. Import onto the `upstream` branch:
   ```
   git rm -rf \*
   7z x /tmp/7z<VVER>-src.7z
   git add -A .
   git commit -F <message-file>
   ```
   The commit message must follow the established pattern:
   ```
   Imported from 7z<VVER>-src.7z.

   This trick was performed by:

     f ()
     {
         git rm -rf \* && 7z x "$1" && git add -A . && git commit ...
     }

     f /path/to/7z<VVER>-src.7z

   Signed-off-by: Johannes Schindelin <johannes.schindelin@gmx.de>
   ```

### Step 4: Rebase patches

1. Create the new branch from the latest version branch:
   ```
   git checkout origin/v<PREV>-VS2022-sfx -b v<VERSION>-VS2022-sfx
   ```
2. Rebase onto the updated upstream:
   ```
   git rebase --rebase-merges upstream
   ```
   **Critical:** The patch series contains merge commits from topic
   branches. Always use `--rebase-merges` to preserve the merge
   structure.
3. If conflicts occur, **stop and report them to the user**. Do not
   attempt to resolve conflicts automatically.
4. Verify the rebase with:
   ```
   git range-diff --remerge-diff upstream origin/v<PREV>-VS2022-sfx HEAD
   ```
   All commits should show `=` (identical patches). If any show
   differences, **stop and present the range-diff to the user**.
5. Verify the graph structure is preserved:
   ```
   git log --oneline --graph HEAD | head -30
   ```
   Compare visually with the original branch to confirm the merge
   topology is identical.

### Step 5: Local build verification

If running in WSL or on Windows with Visual Studio available:

1. Find MSBuild. In WSL:
   ```
   MSBUILD="/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
   ```
   If not at this path, use `vswhere.exe` to locate it. On native
   Windows, use the equivalent Windows path.
2. In WSL, MSBuild cannot write intermediate files on the WSL
   filesystem. Create a temporary worktree on a Windows-native path:
   ```
   TEMP=$(cmd.exe /C "echo %TEMP%" 2>/dev/null | tr -d '\r')
   git worktree add --detach "/mnt/c${TEMP#C:}/7-Zip-build" HEAD
   ```
   On native Windows this is not necessary; build in place or use
   any local path.
3. Build both configurations:
   ```
   "$MSBUILD" /m /p:Configuration=Release <SLN>
   "$MSBUILD" /m /p:Configuration=ReleaseD <SLN>
   ```
   where `<SLN>` is the path to
   `CPP/7zip/Bundles/SFXSetup/SFXSetup.sln`.
4. Verify both `7zS.sfx` and `7zSD.sfx` were produced.
5. Clean up any worktree: `git worktree remove --force <path>`

If Visual Studio is not available at all, skip this step; the CI
build will verify.

### Step 6: Push to git-for-windows/7-Zip -- APPROVAL REQUIRED

**Present the range-diff and graph to the user and ask for explicit
approval before pushing.**

Once approved:

1. Push both branches:
   ```
   git push origin upstream v<VERSION>-VS2022-sfx
   ```
2. The GitHub Actions workflow `msbuild.yml` triggers automatically
   on push and builds two artifacts: `7zS` and `7zSD`.
3. **While waiting for CI:** start cloning `git-for-windows/build-extra`
   (shallow, `--depth 1`) to overlap the wait time.
4. Poll the workflow run via the GitHub API until it completes.
   The run typically takes 1-2 minutes.
5. If the CI run fails, **stop and report the failure to the user**.

### Step 7: Update build-extra -- APPROVAL REQUIRED

Prepare the commit and PR, then ask for approval before pushing and
creating the PR (both happen in one approval step).

1. If not already cloned, clone `git-for-windows/build-extra`
   (`--depth 1`).
2. Create a branch: `git checkout -b 7zip-<VERSION>`
3. Download the CI artifacts (into `/tmp` or similar):
   ```
   cd /tmp && gh run download <RUN_ID> --repo git-for-windows/7-Zip
   ```
   This creates `7zS/7zS.sfx` and `7zSD/7zSD.sfx`.
4. Copy the artifacts into build-extra:
   ```
   cp /tmp/7zS/7zS.sfx 7-Zip/7zS.sfx
   cp /tmp/7zSD/7zSD.sfx 7-Zip/7zSD.sfx
   ```
5. Commit with message (write to file, use `git commit -F`):
   ```
   7z.sfx: update to v<VERSION>

   This corresponds to the commit <TIP_SHA> (<TIP_SUBJECT>,
   <TIP_DATE>) of
   https://github.com/git-for-windows/7-Zip/commits/v<VERSION>-VS2022-sfx
   and the artifacts have been copied from the workflow run at
   https://github.com/git-for-windows/7-Zip/actions/runs/<RUN_ID>.

   This closes https://github.com/git-for-windows/git/issues/<ISSUE>.

   Signed-off-by: Johannes Schindelin <johannes.schindelin@gmx.de>
   ```
   where `<TIP_SHA>`, `<TIP_SUBJECT>`, and `<TIP_DATE>` come from
   the tip commit of `v<VERSION>-VS2022-sfx`.

**Present the commit diff and PR body to the user, then ask for
approval.** Once approved, push and create the PR in one step:

6. Push and create PR:
   ```
   git push origin 7zip-<VERSION>
   gh pr create --repo git-for-windows/build-extra \
     --head 7zip-<VERSION> \
     --title "7z.sfx: update to v<VERSION>" \
     --body-file <file>
   ```
   The PR body matches the commit message body (without the
   Signed-off-by trailer). Include release notes if available.

### Step 8: MINGW-packages (if needed)

This track is independent of Steps 3-7 and can run in parallel.
Check whether `msys2/MINGW-packages` already has the update merged.
If it does, skip this step.

If not:

1. Shallow sparse clone:
   ```
   git clone --depth 1 --sparse --origin upstream \
     https://github.com/msys2/MINGW-packages.git
   cd MINGW-packages
   git sparse-checkout add mingw-w64-7zip
   git remote add origin https://github.com/dscho/MINGW-packages.git
   ```
2. Update `mingw-w64-7zip/PKGBUILD`:
   - `pkgver`: new version (e.g. `26.00`)
   - `pkgrel`: reset to `1`
   - `sha256sums`: download the source tarball and compute:
     ```
     curl -L -o "7z<VVER>-src.tar.xz" \
       "https://7-zip.org/a/7z<VVER>-src.tar.xz"
     sha256sum "7z<VVER>-src.tar.xz"
     ```
     Only update the first hash (the source tarball). Leave patch
     hashes unchanged unless patches were modified.
3. Commit, push, open PR:
   ```
   git checkout -b 7zip-<VERSION>
   git add mingw-w64-7zip/PKGBUILD
   git commit -F <file>
   git push origin 7zip-<VERSION>
   gh pr create --repo msys2/MINGW-packages \
     --head dscho:7zip-<VERSION> \
     --title "7zip: update to <VERSION>" \
     --body-file <file>
   ```

### Step 9: Post-merge follow-up

After the build-extra PR is merged:

1. Update the default branch of `git-for-windows/7-Zip`:
   ```
   gh api repos/git-for-windows/7-Zip -X PATCH \
     -f default_branch=v<VERSION>-VS2022-sfx
   ```

## Approval Gates

The agent must pause and ask for explicit user approval at these
points only:

1. **Before pushing to `git-for-windows/7-Zip`** (Step 6) -- present
   the range-diff and graph comparison first.
2. **Before pushing and creating the build-extra PR** (Step 7) --
   present the commit diff and PR body. The push and `gh pr create`
   happen together after a single approval.
3. **Rebase conflicts** -- if the rebase produces conflicts, stop
   immediately and present the conflict details.

Everything else (cloning, importing, downloading, building,
committing, CI polling) proceeds autonomously.

## Error Handling

- **7z not installed:** Install with `sudo apt-get install -y p7zip-full`.
- **Rebase conflicts:** Stop, show conflicting files, ask user.
- **CI failure:** Stop, show job logs, ask user.
- **Push permission denied:** Stop, inform user.
- **Artifact download fails:** Retry once, then stop.

## Environment Notes

- The agent may run in WSL (check `/proc/version` for `microsoft`)
  or on native Windows. Either works; MSBuild just needs to be
  reachable.
- In WSL, a worktree on a Windows-native path is needed for building
  because MSBuild cannot write intermediate files on the WSL
  filesystem.
- If Visual Studio is not available at all, skip the local build and
  rely on CI.
- `gh` CLI must be authenticated with push access to the
  `git-for-windows` org.
- `7z` must be available for extracting the source archive.
