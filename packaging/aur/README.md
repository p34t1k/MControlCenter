# Arch / AUR packaging

Two PKGBUILDs, same files as the .deb/.rpm (they use the project's CMake `install()` rules):

- `mcenter/` builds the tagged release (bump `pkgver` and the sha256 with `updpkgsums` on each release).
- `mcenter-git/` builds the latest `main`.

Status: the PKGBUILDs pass `bash -n` and the build + `cmake --install` steps were run into a staging directory on Kali, but **`makepkg`/`namcap` have not been run on a real Arch system** and nothing is published on the AUR yet. Test with `cd packaging/aur/mcenter-git && makepkg -si`, and run `namcap PKGBUILD *.pkg.tar.zst`.

To publish: create an AUR account, add an SSH key, then `git clone ssh://aur@aur.archlinux.org/mcenter-git.git`, copy `PKGBUILD`, `mcenter.install` and a generated `.SRCINFO` (`makepkg --printsrcinfo > .SRCINFO`), commit and push.

The `msi-ec` kernel driver is an optional dependency (`msi-ec-dkms` on the AUR) because newer kernels may ship it; the packaged `mcenter-doctor` service checks and repairs it at boot.
