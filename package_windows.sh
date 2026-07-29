#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<EOF
Usage: $0 /path/to/main.exe [output_dir]

This script collects DLL dependencies for a MinGW-built Windows executable
when run in an MSYS2 / MinGW shell. It copies `main.exe` and required
mingw64 DLLs into an output folder and creates a zip archive.

Example (MSYS2 MinGW64 shell):
  ./package_windows.sh /c/Path/To/build/main.exe dist

EOF
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

EXE="$1"
OUTDIR="${2:-dist}"

if [ ! -f "$EXE" ]; then
    echo "Error: executable not found: $EXE" >&2
    exit 2
fi

if [ -d "$OUTDIR" ]; then
    # remove previous contents to avoid stale system DLLs
    if command -v find >/dev/null 2>&1; then
        find "$OUTDIR" -mindepth 1 -exec rm -rf {} + || true
    else
        rm -rf "$OUTDIR"/* || true
    fi
else
    mkdir -p "$OUTDIR"
fi

# copy the exe into a clean output directory
cp -v "$EXE" "$OUTDIR/"

echo "Scanning dependencies..."

deps=()

if command -v ldd >/dev/null 2>&1; then
    # ldd on MSYS2 prints full paths for mingw DLLs
    while IFS= read -r line; do
        # lines like: libfoo.dll => /mingw64/bin/libfoo.dll (0x...)
        if echo "$line" | grep -q '=>'; then
            path=$(echo "$line" | sed -E 's/.*=>[[:space:]]*([^[:space:]]+).*/\1/')
            deps+=("$path")
        else
            # try to extract absolute paths like /mingw64/bin/foo.dll or C:\path\to\foo.dll
            path=$(echo "$line" | grep -oE '(/[A-Za-z0-9._/+-]+\.dll|[A-Za-z]:\\\\[^[:space:]]+\.dll)' || true)
            if [ -n "$path" ]; then
                deps+=("$path")
            fi
        fi
    done < <(ldd "$EXE" 2>/dev/null || true)
fi

if [ ${#deps[@]} -eq 0 ]; then
    # Fallback: use objdump to list imported DLL names
    if command -v objdump >/dev/null 2>&1; then
        while IFS= read -r dll; do
            # objdump -p prints lines like " DLL Name: libgcc_s_seh-1.dll"
            if [[ "$dll" =~ DLL[[:space:]]Name:[[:space:]]+(.+) ]]; then
                dllname="${BASH_REMATCH[1]}"
                # try to locate in common MSYS2 locations
                for p in "/mingw64/bin/$dllname" "/usr/bin/$dllname"; do
                    if [ -f "$p" ]; then
                        deps+=("$p")
                        break
                    fi
                done
            fi
        done < <(objdump -p "$EXE" 2>/dev/null || true)
    fi
fi

printf "Found %d dependency paths\n" "${#deps[@]}"

# Helper to convert Windows paths like C:\\Foo\\Bar.dll to /c/Foo/Bar.dll
convert_to_unix_path() {
    local p="$1"
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -u "$p" 2>/dev/null || echo "$p"
        return
    fi
    # naive conversion
    if [[ "$p" =~ ^[A-Za-z]:\\\\ ]]; then
        # C:\\path -> /c/path
        p="/${p:0:1}/${p:2}"
        p="${p//\\//}"
        echo "$p"
    else
        echo "$p"
    fi
}

copied=()
declare -A copied_map
for p in "${deps[@]}"; do
    [ -z "$p" ] && continue

    # normalize path (convert Windows-style to unix if needed)
    p_unix="$p"
    if [[ "$p" =~ [A-Za-z]:\\\\ ]]; then
        p_unix=$(convert_to_unix_path "$p")
    fi

    # if file doesn't exist at that path, try common locations
    if [ ! -f "$p_unix" ]; then
        bn=$(basename "$p_unix")
        candidates=(
            "$(dirname "$EXE")/$bn"
            "/mingw64/bin/$bn"
            "/usr/bin/$bn"
        )
        found=""
        for c in "${candidates[@]}"; do
            if [ -f "$c" ]; then
                found="$c"
                break
            fi
        done
        if [ -z "$found" ]; then
            # try PATH
            whichpath=$(which "$bn" 2>/dev/null || true)
            if [ -n "$whichpath" ] && [ -f "$whichpath" ]; then
                found="$whichpath"
            fi
        fi
        if [ -n "$found" ]; then
            p_unix="$found"
        else
            echo "Warning: could not locate $bn; skipping" >&2
            continue
        fi
    fi

    # canonicalize
    if command -v readlink >/dev/null 2>&1; then
        p_unix=$(readlink -f "$p_unix" || echo "$p_unix")
    fi

    lower=$(echo "$p_unix" | tr '[:upper:]' '[:lower:]')

    # Skip Windows system DLLs by path
    if [[ "$lower" == /c/windows/* || "$lower" == *"/windows/system32"* || "$lower" == *"/windows/syswow64"* || "$lower" == *"/windows/system"* ]]; then
        echo "Skipping system DLL: $p_unix"
        continue
    fi

    # Also skip some well-known system DLL basenames just in case
    sysnames=(advapi32.dll kernel32.dll kernelbase.dll user32.dll gdi32.dll shell32.dll rpcrt4.dll ws2_32.dll combase.dll dxcore.dll dwrite.dll usp10.dll winmm.dll opengl32.dll glu32.dll gdi32full.dll)
    bn_lower=$(basename "$p_unix" | tr '[:upper:]' '[:lower:]')
    skip=false
    for s in "${sysnames[@]}"; do
        if [ "$bn_lower" = "$s" ]; then
            skip=true
            break
        fi
    done
    if [ "$skip" = true ]; then
        echo "Skipping known system DLL: $p_unix"
        continue
    fi

    # copy if not already copied
    if [ -z "${copied_map[$p_unix]:-}" ]; then
        echo "Copying $p_unix -> $OUTDIR/"
        cp -v "$p_unix" "$OUTDIR/"
        copied+=("$p_unix")
        copied_map[$p_unix]=1
    fi
done

echo
echo "Copied files:"
if [ ${#copied[@]} -gt 0 ]; then
    for f in "${copied[@]}"; do
        echo " - $(basename "$f")"
    done
else
    for f in "$OUTDIR"/*; do
        [ -e "$f" ] || continue
        echo " - $(basename "$f")"
    done
fi

if command -v zip >/dev/null 2>&1; then
    zipfile="${OUTDIR}.zip"
    echo "Creating $zipfile..."
    (cd "$OUTDIR" && zip -r -q "../$(basename "$zipfile")" .)
    echo "Package created: $zipfile"
else
    echo "zip not found; left package files in $OUTDIR/"
fi

echo "Done. Test the package on a plain Windows machine; if the executable was built with MSVC you will also need the appropriate VC++ Redistributable installer." 
