#!/usr/bin/env python3
"""Generate the small cs2-dumper snapshot used by Very-Paste.

The preferred flow for simv0lofficial/cs2_dumper is:
  1. Run cs2-dumper on Windows while CS2 is open.
  2. Point this script at its output directory:
       python tools/update_offsets.py --source-dir path/to/cs2_dumper/output

If --source-dir is omitted the script tries to fetch public generated output from
known raw GitHub locations. No third-party Python packages are required.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.error
import urllib.request
from collections import OrderedDict
from datetime import datetime, timezone
from pathlib import Path
from typing import Mapping

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = REPO_ROOT / "velocity-cs2" / "project" / "protection" / "cs2_dumper_offsets.hpp"

DEFAULT_BASE_URLS = (
    "https://raw.githubusercontent.com/simv0lofficial/cs2_dumper/main/output",
    "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output",
)

MODULE_NAMESPACE = {
    "client.dll": "client_dll",
    "engine2.dll": "engine2_dll",
    "filesystem_stdio.dll": "filesystem_stdio_dll",
    "inputsystem.dll": "inputsystem_dll",
    "localize.dll": "localize_dll",
    "materialsystem2.dll": "materialsystem2_dll",
    "meshsystem.dll": "meshsystem_dll",
    "panorama.dll": "panorama_dll",
    "particles.dll": "particles_dll",
    "resourcesystem.dll": "resourcesystem_dll",
    "scenesystem.dll": "scenesystem_dll",
    "schemasystem.dll": "schemasystem_dll",
    "soundsystem.dll": "soundsystem_dll",
    "tier0.dll": "tier0_dll",
    "vphysics2.dll": "vphysics2_dll",
}

# Keep this list intentionally focused on values consumed by the project or useful
# for nearby runtime sanity checks. Add new fields here if code starts using more
# cs2-dumper values.
REQUIRED_OFFSETS: "OrderedDict[str, tuple[str, ...]]" = OrderedDict(
    {
        "client_dll": (
            "dwCSGOInput",
            "dwEntityList",
            "dwGameEntitySystem",
            "dwGameEntitySystem_highestEntityIndex",
            "dwGameRules",
            "dwGlobalVars",
            "dwGlowManager",
            "dwLocalPlayerController",
            "dwLocalPlayerPawn",
            "dwPlantedC4",
            "dwPrediction",
            "dwSensitivity",
            "dwSensitivity_sensitivity",
            "dwViewAngles",
            "dwViewMatrix",
            "dwViewRender",
            "dwWeaponC4",
        ),
        "engine2_dll": (
            "dwBuildNumber",
            "dwNetworkGameClient",
            "dwNetworkGameClient_clientTickCount",
            "dwNetworkGameClient_deltaTick",
            "dwNetworkGameClient_isBackgroundMap",
            "dwNetworkGameClient_localPlayer",
            "dwNetworkGameClient_maxClients",
            "dwNetworkGameClient_serverTickCount",
            "dwNetworkGameClient_signOnState",
            "dwWindowHeight",
            "dwWindowWidth",
        ),
        "inputsystem_dll": ("dwInputSystem",),
        "soundsystem_dll": ("dwSoundSystem", "dwSoundSystem_engineViewData"),
    }
)

REQUIRED_BUTTONS = (
    "attack",
    "attack2",
    "back",
    "duck",
    "forward",
    "jump",
    "left",
    "lookatweapon",
    "reload",
    "right",
    "showscores",
    "sprint",
    "turnleft",
    "turnright",
    "use",
    "zoom",
)

REQUIRED_INTERFACES: "OrderedDict[str, tuple[str, ...]]" = OrderedDict(
    {
        "client.dll": (
            "Source2Client002",
            "Source2ClientPrediction001",
            "LegacyGameUI001",
        ),
        "engine2.dll": (
            "Source2EngineToClient001",
            "NetworkClientService_001",
            "GameEventSystemClientV001",
        ),
        "panorama.dll": ("PanoramaUIEngine001",),
        "scenesystem.dll": ("SceneSystem_002",),
        "materialsystem2.dll": ("VMaterialSystem2_001",),
        "schemasystem.dll": ("SchemaSystem_001",),
        "inputsystem.dll": ("InputSystemVersion001", "InputStackSystemVersion001"),
        "particles.dll": ("ParticleSystemMgr003",),
        "tier0.dll": ("VEngineCvar007", "VStringTokenSystem001"),
        "resourcesystem.dll": ("ResourceSystem013",),
        "localize.dll": ("Localize_001",),
        "meshsystem.dll": ("MeshSystem001",),
        "filesystem_stdio.dll": ("VFileSystem017",),
        "soundsystem.dll": ("SoundSystem001",),
        "vphysics2.dll": ("VPhysics2_Interface_001",),
    }
)

CONST_RE = re.compile(
    r"\bconstexpr\s+std::ptrdiff_t\s+([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;"
)
HEADER_TIMESTAMP_RE = re.compile(r"^//\s*(\d{4}-\d{2}-\d{2}[^\n]*?UTC)", re.MULTILINE)


class UpdateError(RuntimeError):
    pass


def parse_int_literal(value: str) -> int:
    return int(value, 16 if value.lower().startswith("0x") else 10)


def format_hex(value: int) -> str:
    return f"0x{value:X}"


def find_namespace_block(text: str, namespace: str) -> str | None:
    match = re.search(rf"\bnamespace\s+{re.escape(namespace)}\s*\{{", text)
    if not match:
        return None

    brace_index = text.find("{", match.start())
    depth = 0
    for index in range(brace_index, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[brace_index + 1 : index]

    raise UpdateError(f"unterminated namespace block: {namespace}")


def parse_namespace_constants(text: str, namespace: str) -> dict[str, int]:
    block = find_namespace_block(text, namespace)
    if block is None:
        return {}
    return {name: parse_int_literal(value) for name, value in CONST_RE.findall(block)}


def require_values(
    parsed: Mapping[str, Mapping[str, int]],
    required: Mapping[str, tuple[str, ...]],
    label: str,
) -> "OrderedDict[str, OrderedDict[str, int]]":
    result: "OrderedDict[str, OrderedDict[str, int]]" = OrderedDict()
    missing: list[str] = []

    for namespace, names in required.items():
        namespace_values = parsed.get(namespace, {})
        filtered: "OrderedDict[str, int]" = OrderedDict()
        for name in names:
            if name not in namespace_values:
                missing.append(f"{label}.{namespace}.{name}")
                continue
            filtered[name] = namespace_values[name]
        result[namespace] = filtered

    if missing:
        joined = "\n  - ".join(missing)
        raise UpdateError(f"missing required {label} values:\n  - {joined}")

    return result


def read_local_sources(source_dir: Path) -> tuple[dict[str, str], str]:
    files: dict[str, str] = {}
    for name in ("offsets.hpp", "buttons.hpp", "interfaces.hpp"):
        path = source_dir / name
        if not path.is_file():
            raise UpdateError(f"{path} not found")
        files[name] = path.read_text(encoding="utf-8", errors="replace")

    info_path = source_dir / "info.json"
    if info_path.is_file():
        files["info.json"] = info_path.read_text(encoding="utf-8", errors="replace")

    return files, str(source_dir)


def fetch_url(url: str) -> str:
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "Very-Paste-offset-updater/1.0",
            "Accept": "text/plain, application/json;q=0.9, */*;q=0.1",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read().decode("utf-8", errors="replace")


def fetch_remote_sources(base_url: str) -> dict[str, str]:
    base = base_url.rstrip("/")
    files: dict[str, str] = {}
    for name in ("offsets.hpp", "buttons.hpp", "interfaces.hpp"):
        files[name] = fetch_url(f"{base}/{name}")

    try:
        files["info.json"] = fetch_url(f"{base}/info.json")
    except (OSError, urllib.error.URLError, urllib.error.HTTPError):
        pass

    return files


def read_sources(args: argparse.Namespace) -> tuple[dict[str, str], str]:
    if args.source_dir:
        label = args.source_label or "local cs2-dumper output directory"
        files, _ = read_local_sources(Path(args.source_dir))
        return files, label

    errors: list[str] = []
    for base_url in args.base_url or DEFAULT_BASE_URLS:
        try:
            return fetch_remote_sources(base_url), args.source_label or base_url.rstrip("/")
        except Exception as exc:  # noqa: BLE001 - report all candidates to the user.
            errors.append(f"{base_url}: {exc}")

    joined = "\n  - ".join(errors)
    raise UpdateError(f"failed to read cs2-dumper output from all sources:\n  - {joined}")


def parse_generated_at(files: Mapping[str, str]) -> str:
    for name in ("offsets.hpp", "buttons.hpp", "interfaces.hpp"):
        match = HEADER_TIMESTAMP_RE.search(files.get(name, ""))
        if match:
            return match.group(1).strip()

    info_text = files.get("info.json")
    if info_text:
        try:
            info = json.loads(info_text)
        except json.JSONDecodeError:
            info = {}
        timestamp = info.get("timestamp") or info.get("generated_at")
        if isinstance(timestamp, str) and timestamp:
            try:
                dt = datetime.fromisoformat(timestamp.replace("Z", "+00:00"))
                return dt.astimezone(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
            except ValueError:
                return timestamp

    return datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")


def parse_game_build(files: Mapping[str, str], override: int | None) -> int:
    if override is not None:
        return override

    info_text = files.get("info.json")
    if not info_text:
        return 0

    try:
        info = json.loads(info_text)
    except json.JSONDecodeError:
        return 0

    for key in ("build_number", "game_build_number", "game_build", "build"):
        value = info.get(key)
        if isinstance(value, int):
            return value
        if isinstance(value, str) and value.isdigit():
            return int(value)

    return 0


def parse_all(files: Mapping[str, str]) -> tuple[
    "OrderedDict[str, OrderedDict[str, int]]",
    "OrderedDict[str, int]",
    "OrderedDict[str, OrderedDict[str, int]]",
]:
    parsed_offsets = {
        namespace: parse_namespace_constants(files["offsets.hpp"], namespace)
        for namespace in REQUIRED_OFFSETS
    }
    offsets = require_values(parsed_offsets, REQUIRED_OFFSETS, "offsets")

    button_values = parse_namespace_constants(files["buttons.hpp"], "buttons")
    missing_buttons = [name for name in REQUIRED_BUTTONS if name not in button_values]
    if missing_buttons:
        joined = "\n  - ".join(missing_buttons)
        raise UpdateError(f"missing required button values:\n  - {joined}")
    buttons: "OrderedDict[str, int]" = OrderedDict((name, button_values[name]) for name in REQUIRED_BUTTONS)

    interface_parsed = {
        MODULE_NAMESPACE[module_name]: parse_namespace_constants(
            files["interfaces.hpp"], MODULE_NAMESPACE[module_name]
        )
        for module_name in REQUIRED_INTERFACES
    }
    interface_required = OrderedDict(
        (MODULE_NAMESPACE[module_name], names)
        for module_name, names in REQUIRED_INTERFACES.items()
    )
    interfaces = require_values(interface_parsed, interface_required, "interfaces")

    return offsets, buttons, interfaces


def generate_header(
    offsets: Mapping[str, Mapping[str, int]],
    buttons: Mapping[str, int],
    interfaces: Mapping[str, Mapping[str, int]],
    *,
    source_label: str,
    generated_at: str,
    game_build: int,
) -> str:
    lines: list[str] = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "#include <cstdint>",
        "#include <string_view>",
        "",
        "// This file is generated by tools/update_offsets.py. Do not edit manually.",
        "// Source requested by project owner: https://github.com/simv0lofficial/cs2_dumper",
        f"// Parsed source: {source_label}",
        f"// Generated metadata timestamp: {generated_at}",
        "// Only offsets used by this project are mirrored here.",
        "",
        "namespace cs2_dumper::generated {",
        "",
        f"\tinline constexpr std::string_view generated_at{{ \"{generated_at}\" }};",
        f"\tinline constexpr int game_build{{ {game_build} }};",
        "",
        "\tnamespace offsets {",
        "",
    ]

    for namespace, values in offsets.items():
        lines.append(f"\t\tnamespace {namespace} {{")
        for name, value in values.items():
            lines.append(f"\t\t\tconstexpr std::ptrdiff_t {name} = {format_hex(value)};")
        lines.append(f"\t\t}} // namespace {namespace}")
        lines.append("")

    lines.extend([
        "\t} // namespace offsets",
        "",
        "\tnamespace buttons {",
    ])

    for name, value in buttons.items():
        lines.append(f"\t\tconstexpr std::ptrdiff_t {name} = {format_hex(value)};")

    lines.extend([
        "\t} // namespace buttons",
        "",
        "\tnamespace interfaces {",
        "",
    ])

    for namespace, values in interfaces.items():
        lines.append(f"\t\tnamespace {namespace} {{")
        for name, value in values.items():
            lines.append(f"\t\t\tconstexpr std::ptrdiff_t {name} = {format_hex(value)};")
        lines.append(f"\t\t}} // namespace {namespace}")
        lines.append("")

    lines.extend(
        [
            "\t\t[[nodiscard]] inline std::ptrdiff_t offset(",
            "\t\t\tstd::string_view module_name,",
            "\t\t\tstd::string_view interface_name ) noexcept",
            "\t\t{",
        ]
    )

    first = True
    for module_name, names in REQUIRED_INTERFACES.items():
        namespace = MODULE_NAMESPACE[module_name]
        prefix = "if" if first else "else if"
        first = False
        lines.append(f"\t\t\t{prefix} ( module_name == \"{module_name}\" )")
        lines.append("\t\t\t{")
        for name in names:
            lines.append(f"\t\t\t\tif ( interface_name == \"{name}\" ) return {namespace}::{name};")
        lines.append("\t\t\t}")

    lines.extend(
        [
            "",
            "\t\t\treturn 0;",
            "\t\t}",
            "",
            "\t\t[[nodiscard]] inline std::uintptr_t address(",
            "\t\t\tstd::uintptr_t module_base,",
            "\t\t\tstd::string_view module_name,",
            "\t\t\tstd::string_view interface_name ) noexcept",
            "\t\t{",
            "\t\t\tconst auto rva = offset( module_name, interface_name );",
            "\t\t\treturn module_base && rva > 0",
            "\t\t\t\t? module_base + static_cast< std::uintptr_t >( rva )",
            "\t\t\t\t: 0;",
            "\t\t}",
            "",
            "\t} // namespace interfaces",
            "",
            "} // namespace cs2_dumper::generated",
            "",
        ]
    )

    return "\n".join(lines)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-dir",
        type=Path,
        help="Directory containing cs2-dumper output files (offsets.hpp/buttons.hpp/interfaces.hpp/info.json).",
    )
    parser.add_argument(
        "--base-url",
        action="append",
        help="Raw base URL containing output files. Can be specified multiple times.",
    )
    parser.add_argument(
        "--source-label",
        help="Human-readable source label to write into the generated header.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Header output path (default: {DEFAULT_OUTPUT.relative_to(REPO_ROOT)}).",
    )
    parser.add_argument(
        "--game-build",
        type=int,
        help="Override build number when info.json is unavailable or stale.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Do not write; fail if the generated output differs from the existing file.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    try:
        files, source_label = read_sources(args)
        offsets, buttons, interfaces = parse_all(files)
        generated_at = parse_generated_at(files)
        game_build = parse_game_build(files, args.game_build)
        header = generate_header(
            offsets,
            buttons,
            interfaces,
            source_label=source_label,
            generated_at=generated_at,
            game_build=game_build,
        )

        output = args.output
        if args.check:
            existing = output.read_text(encoding="utf-8") if output.exists() else ""
            if existing != header:
                raise UpdateError(f"{output} is not up to date")
            print(f"{output} is up to date")
            return 0

        output.parent.mkdir(parents=True, exist_ok=True)
        old = output.read_text(encoding="utf-8") if output.exists() else None
        if old == header:
            print(f"{output} already up to date")
        else:
            output.write_text(header, encoding="utf-8", newline="\n")
            print(f"wrote {output}")
        return 0

    except UpdateError as exc:
        parser.exit(1, f"error: {exc}\n")


if __name__ == "__main__":
    raise SystemExit(main())
