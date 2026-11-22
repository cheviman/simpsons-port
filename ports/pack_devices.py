"""
    Name:        pack_devices.py
    Description: the release's packages of the devices, one each: simpsons-switch.zip, simpsons-android.zip,
                 simpsons-anbernic.zip and simpsons-ios.zip, each the device's games without assets, beside the
                 tool that makes the player's games of them for Windows, macOS and Linux where the device has one
                 (as tmnt-port's)
    Version:     0.1.0
    Created:     13-11-2026
    Modified:    13-11-2026
    Author:      Sebastian Toyos

    license:BSD-3-Clause
    A package is a folder (simpsons-switch, ...) holding the device's README, the tool three times (its Windows
    .exe, and its macOS and Linux programs with the system in their names: the three sit beside the device's
    folder, which each finds there), the device's games in its folder (switch, android, anbernic), font.bmp
    (the tools' letters) and the licenses. The tools, font.bmp and the licenses come from each computer's kit
    (the CI's build job: dist/kit, packed as simpsons-OS-kit.tar.gz), given as a folder each under --kits:
    windows-x64, macos-universal and linux-x64. The zip keeps the programs' permissions (Unix entries), so
    that the macOS and Linux tools run as they are unpacked there.

    The Simpsons' games also take their assets out of the player's simpsons.zip on the device itself
    (ports/sdl2/main.c's device_assets), so a package carries them ready as well, beside the tool's way: the
    Switch's .nro in sd/switch/simpsons-arcade/ (--switch-nro), Android's APKs signed with the project's key in ready/
    (--android-ready, when the CI has the key), and Anbernic's folder anbernic/, which is the card's Roms/PORTS as
    it is. A README's line marked {ready} is there only with the signed APKs, one marked {no-ready} only without
    them.

    iOS has no tool: simpsons-ios.zip holds the two unsigned .ipa (--ios, the CI's ios job: ports/ios/ios.cmake),
    which the player signs as they are installed (AltStore, SideStore, Sideloadly), with its README and the
    licenses.

    python3 ports/pack_devices.py --kits DIR --switch DIR --switch-nro DIR --android DIR [--android-ready DIR]
        --anbernic DIR [--ios DIR] --out DIR
"""
import argparse
import os
import stat
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs", "package")

# device, the tool's name, the package's README
DEVICES = [
    ("switch", "Simpsons NSP", "README-switch.txt"),
    ("android", "Simpsons APK", "README-android.txt"),
    ("anbernic", "Simpsons Anbernic", "README-anbernic.txt"),
]

# each computer's kit: its folder under --kits, the tool's name in it and in the package
KITS = [
    ("windows-x64", "{tool}.exe", "{tool}.exe"),
    ("macos-universal", "{tool}", "{tool} (macOS)"),
    ("linux-x64", "{tool}", "{tool} (Linux)"),
]

# the programs in the devices' folders (Anbernic's executables and launchers)
DEVICE_PROGRAMS = ("The Simpsons Mr. Plow Edition", "The Simpsons Arcade Game", "The Simpsons Mr. Plow Edition.sh", "The Simpsons Arcade Game.sh")

PROGRAM = 0o755
FILE = 0o644


def add(archive, source, name, mode):
    """A file into the zip as a Unix entry with 'mode', so that unzip and macOS's Archive Utility keep it"""
    info = zipfile.ZipInfo.from_file(source, name)
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    with open(source, "rb") as data:
        archive.writestr(info, data.read(), compresslevel=9)


def add_folder(archive, source, name, programs=(), suffix=None, leave=(".nro", "-debug.apk", ".idsig", ".assets")):
    """A folder's files into the zip under 'name', in a stable order; 'programs' the names that run; 'suffix'
    (".nro", ".apk", ".ipa") the files taken, all of them without one; 'leave' the ends of those never taken there
    (a computer's own .nro or -debug.apk carries its assets)"""
    for root, folders, files in os.walk(source):
        folders.sort()
        for file in sorted(files):
            if (suffix and not file.endswith(suffix)) or (not suffix and file.endswith(leave)):
                continue
            path = os.path.join(root, file)
            inside = name + "/" + os.path.relpath(path, source).replace(os.sep, "/")
            add(archive, path, inside, PROGRAM if file in programs else FILE)


def add_readme(archive, source, name, kept):
    """The device's README into the zip: a line marked {mark} only when 'mark' is in 'kept' (the extras' folders
    in the package: "ready"), {no-mark} only when it is not, both without their mark"""
    lines = []
    with open(source, encoding="utf-8", newline="") as text:
        for line in text.read().splitlines(keepends=True):
            if line.startswith("{"):
                mark, line = line[1:].split("}", 1)
                if (mark[3:] in kept) if mark.startswith("no-") else (mark not in kept):
                    continue
            lines.append(line)
    info = zipfile.ZipInfo.from_file(source, name)
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | FILE) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(info, "".join(lines).encode("utf-8"), compresslevel=9)


def need(path):
    if not os.path.exists(path):
        sys.exit("pack_devices: missing " + path)
    return path


def forbidden(names):
    """What never goes in a package: an assets pack, a debug APK (this computer's, with its assets), the assets'
    folder, a ROM set"""
    return [name for name in names if name.endswith(".assets") or name.endswith("-debug.apk")
            or "/assets/" in name or name.endswith(".zip")]


def shared_files(archive, top, kits):
    """The project's LICENSE.txt and the licenses (the same in every kit: Linux's)"""
    shared = os.path.join(kits, "linux-x64", "kit")
    add(archive, need(os.path.join(shared, "LICENSE.txt")), top + "/LICENSE.txt", FILE)
    add_folder(archive, need(os.path.join(shared, "licenses")), top + "/licenses")
    return shared


def finish(target):
    """The zip read back: nothing of the ROM set in it, or no zip"""
    with zipfile.ZipFile(target) as archive:
        names = archive.namelist()
    wrong = forbidden(names)
    if wrong:
        os.remove(target)
        sys.exit("pack_devices: %s would carry %s" % (target, ", ".join(wrong)))
    print("%s: %d files, %d bytes" % (target, len(names), os.path.getsize(target)))
    for name in names:
        print("   " + name)


def pack(device, tool, readme, game, extras, kits, out):
    top = "simpsons-" + device
    target = os.path.join(out, top + ".zip")
    with zipfile.ZipFile(target, "w") as archive:
        kept = {inside for _, inside, _ in extras}
        add_readme(archive, need(os.path.join(DOCS, readme)), top + "/README.txt", kept)
        shared = shared_files(archive, top, kits)
        add(archive, need(os.path.join(shared, "font.bmp")), top + "/font.bmp", FILE)
        for kit, name, packed in KITS:
            source = need(os.path.join(kits, kit, "kit", name.format(tool=tool)))
            add(archive, source, top + "/" + packed.format(tool=tool), PROGRAM)
        add_folder(archive, need(game), top + "/" + device, programs=DEVICE_PROGRAMS)
        for source, inside, suffix in extras:
            add_folder(archive, source, top + "/" + inside, suffix=suffix)
    finish(target)


def pack_ios(ipas, kits, out):
    """simpsons-ios.zip: the README, the licenses and ios/ with the two .ipa"""
    top = "simpsons-ios"
    target = os.path.join(out, top + ".zip")
    found = sorted(name for name in os.listdir(need(ipas)) if name.endswith(".ipa"))
    if found != ["The Simpsons Arcade Game.ipa", "The Simpsons Mr. Plow Edition.ipa"]:
        sys.exit("pack_devices: %s has %s, not The Simpsons Mr. Plow Edition.ipa and The Simpsons Arcade Game.ipa" % (ipas, found))
    with zipfile.ZipFile(target, "w") as archive:
        add_readme(archive, need(os.path.join(DOCS, "README-ios.txt")), top + "/README.txt", set())
        shared_files(archive, top, kits)
        add_folder(archive, ipas, top + "/ios", suffix=".ipa")
    finish(target)


def main():
    parser = argparse.ArgumentParser(description="The devices' packages of a release")
    parser.add_argument("--kits", required=True, help="the computers' kits: windows-x64, macos-universal, linux-x64")
    parser.add_argument("--switch", required=True,
                        help="the Switch's games: simpsons/ and simpsons-arcade/, the .nsp's pieces")
    parser.add_argument("--switch-nro", required=True,
                        help="the Switch's .nro without assets: simpsons.nro (The Simpsons Mr. Plow Edition), simpsons-arcade.nro (The Simpsons Arcade Game)")
    parser.add_argument("--android", required=True,
                        help="Android's games: simpsons-base.apk, simpsons-arcade-base.apk")
    parser.add_argument("--android-ready", help="Android's APKs signed with the project's key (none: not in it)")
    parser.add_argument("--anbernic", required=True,
                        help="Anbernic's games: The Simpsons Mr. Plow Edition.sh, The Simpsons Arcade Game.sh and simpsons/")
    parser.add_argument("--ios", help="iOS's games: The Simpsons Mr. Plow Edition.ipa, The Simpsons Arcade Game.ipa (none: no iOS package)")
    parser.add_argument("--out", required=True, help="where the zips go")
    arguments = parser.parse_args()
    os.makedirs(arguments.out, exist_ok=True)
    games = {"switch": arguments.switch, "android": arguments.android, "anbernic": arguments.anbernic}
    extras = {"switch": [(need(arguments.switch_nro), "sd/switch/simpsons-arcade", ".nro")], "android": [],
              "anbernic": []}
    if arguments.android_ready and os.path.isdir(arguments.android_ready) \
            and any(name.endswith(".apk") for name in os.listdir(arguments.android_ready)):
        extras["android"].append((arguments.android_ready, "ready", ".apk"))
    else:
        print("pack_devices: no signed APKs (the repository has no key): simpsons-android.zip with the tool's way "
              "alone")
    for device, tool, readme in DEVICES:
        pack(device, tool, readme, games[device], extras[device], arguments.kits, arguments.out)
    if arguments.ios:
        pack_ios(arguments.ios, arguments.kits, arguments.out)


if __name__ == "__main__":
    main()
