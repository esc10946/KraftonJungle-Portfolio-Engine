from dataclasses import dataclass
import os
from pathlib import Path
import re
import sys


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parents[1]
GENERATED_DIR = PROJECT_DIR / "Generated"
GENERATED_CPP = GENERATED_DIR / "JSEngine.generated.cpp"

TYPE_MAP = {
    "bool": "EReflectedPropertyType::Bool",
    "int32": "EReflectedPropertyType::Int32",
    "float": "EReflectedPropertyType::Float",
    "FName": "EReflectedPropertyType::Name",
    "FString": "EReflectedPropertyType::String",
}

FLAG_MAP = {
    "Read": "EPropertyFlags::Read",
    "Write": "EPropertyFlags::Write",
    "Transient": "EPropertyFlags::Transient",
    "LuaRead": "EPropertyFlags::LuaRead",
    "LuaWrite": "EPropertyFlags::LuaWrite",
}

CLASS_PATTERN = re.compile(
    r"UCLASS\s*\(\s*\)\s*\n\s*class\s+(?P<class>\w+)\s*:\s*public\s+(?P<super>\w+)",
    re.MULTILINE,
)

PROPERTY_PATTERN = re.compile(
    r"UPROPERTY\s*\((?P<flags>[^)]*)\)\s*\n\s*"
    r"(?P<type>[\w:]+(?:\s*\*)?)\s+"
    r"(?P<name>\w+)\s*(?:=[^;]*)?;",
    re.MULTILINE,
)


@dataclass
class ReflectedProperty:
    name: str
    cpp_type: str
    reflected_type: str
    flags: str
    object_class: str | None = None


@dataclass
class ReflectedClass:
    file_path: Path
    class_name: str
    super_name: str
    class_flags: str
    properties: list[ReflectedProperty]


class ReflectionGeneratorError(Exception):
    pass


def should_skip(path: Path) -> bool:
    rel_parts = {part.lower() for part in path.relative_to(PROJECT_DIR).parts}
    return (
        "generated" in rel_parts
        or "thirdparty" in rel_parts
        or "build" in rel_parts
        or "bin" in rel_parts
        or "packages" in rel_parts
        or path.name.endswith(".generated.h")
    )


def line_number(content: str, offset: int) -> int:
    return content.count("\n", 0, offset) + 1


def fail(path: Path, content: str, offset: int, message: str) -> None:
    rel_path = path.relative_to(PROJECT_DIR)
    raise ReflectionGeneratorError(
        f"{rel_path}:{line_number(content, offset)}: {message}"
    )


def normalize_cpp_type(cpp_type: str) -> str:
    return re.sub(r"\s+", "", cpp_type.strip())


def reflected_type(cpp_type: str) -> tuple[str | None, str | None]:
    if cpp_type in TYPE_MAP:
        return TYPE_MAP[cpp_type], None

    if cpp_type.endswith("*"):
        class_name = cpp_type[:-1]
        if class_name.startswith(("U", "A")):
            return "EReflectedPropertyType::Object", class_name

    return None, None


def reflected_type_for_flags(
    path: Path,
    content: str,
    offset: int,
    cpp_type: str,
    flag_parts: list[str],
) -> tuple[str | None, str | None]:
    if "Asset" in flag_parts:
        if cpp_type != "FString":
            fail(path, content, offset, "UPROPERTY Asset must be declared as FString path")
        return "EReflectedPropertyType::Asset", None

    return reflected_type(cpp_type)


def parse_flags(path: Path, content: str, offset: int, flag_text: str) -> str:
    parts = [part.strip() for part in flag_text.split(",") if part.strip()]
    flags: list[str] = []

    if "Edit" in parts:
        flags.extend(
            [
                "EPropertyFlags::Read",
                "EPropertyFlags::Write",
                "EPropertyFlags::Edit",
            ]
        )

    for part in parts:
        if part == "Edit" or part == "Asset":
            continue
        mapped_flag = FLAG_MAP.get(part)
        if not mapped_flag:
            fail(path, content, offset, f"unsupported UPROPERTY flag '{part}'")
        flags.append(mapped_flag)

    if not flags:
        flags.append("EPropertyFlags::None")

    result: list[str] = []
    for flag in flags:
        if flag not in result:
            result.append(flag)

    return " |\n                                ".join(result)


def find_matching_class_body(path: Path, content: str, class_match: re.Match) -> tuple[int, int]:
    open_brace = content.find("{", class_match.end())
    if open_brace == -1:
        fail(path, content, class_match.start(), "reflected class has no body")

    depth = 0
    for index in range(open_brace, len(content)):
        char = content[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                if content[index + 1 : index + 2] != ";":
                    fail(path, content, index, "reflected class body must end with '};'")
                return open_brace + 1, index

    fail(path, content, class_match.start(), "reflected class body is not closed")
    raise AssertionError("unreachable")


def strip_conditional_blocks(body: str) -> str:
    stripped_lines: list[str] = []
    conditional_depth = 0

    for line in body.splitlines(keepends=True):
        directive = line.lstrip()
        if re.match(r"#\s*(if|ifdef|ifndef)\b", directive):
            conditional_depth += 1
            stripped_lines.append("\n")
            continue
        if re.match(r"#\s*endif\b", directive):
            conditional_depth = max(0, conditional_depth - 1)
            stripped_lines.append("\n")
            continue

        if conditional_depth > 0:
            stripped_lines.append("\n")
        else:
            stripped_lines.append(line)

    return "".join(stripped_lines)


def infer_class_flags(class_name: str, super_name: str) -> str:
    if class_name.startswith("A") or super_name.startswith("A"):
        return "CF_Actor"
    if class_name.endswith("Component") or super_name.endswith("Component"):
        return "CF_Component"
    return "CF_None"


def parse_header(path: Path) -> ReflectedClass | None:
    content = path.read_text(encoding="utf-8")

    class_matches = list(CLASS_PATTERN.finditer(content))
    if not class_matches:
        return None
    if len(class_matches) > 1:
        fail(path, content, class_matches[1].start(), "only one UCLASS per header is supported")

    class_match = class_matches[0]
    class_name = class_match.group("class")
    super_name = class_match.group("super")

    expected_include = f'#include "{class_name}.generated.h"'
    if expected_include not in content:
        rel_path = path.relative_to(PROJECT_DIR)
        print(f"[ReflectionGenerator] {rel_path.name}: missing {expected_include}", file=sys.stderr)
        # 에러로 중단하려면:
        fail(path, content, class_match.start(), f"missing {expected_include}")


    body_start, body_end = find_matching_class_body(path, content, class_match)
    body = content[body_start:body_end]
    parseable_body = strip_conditional_blocks(body)

    properties: list[ReflectedProperty] = []
    property_names: set[str] = set()
    for prop_match in PROPERTY_PATTERN.finditer(parseable_body):
        absolute_offset = body_start + prop_match.start()
        flag_text = prop_match.group("flags")
        raw_cpp_type = prop_match.group("type")
        cpp_type = normalize_cpp_type(raw_cpp_type)
        prop_name = prop_match.group("name")

        if prop_name in property_names:
            fail(path, content, absolute_offset, f"duplicate UPROPERTY name '{prop_name}'")
        property_names.add(prop_name)

        if "<" in raw_cpp_type or ">" in raw_cpp_type:
            fail(path, content, absolute_offset, "template UPROPERTY types are not supported")

        flag_parts = [part.strip() for part in flag_text.split(",") if part.strip()]
        prop_type, object_class = reflected_type_for_flags(path, content, absolute_offset, cpp_type, flag_parts)
        if not prop_type:
            fail(path, content, absolute_offset, f"unsupported UPROPERTY type '{raw_cpp_type.strip()}'")

        properties.append(
            ReflectedProperty(
                name=prop_name,
                cpp_type=cpp_type,
                reflected_type=prop_type,
                flags=parse_flags(path, content, absolute_offset, flag_text),
                object_class=object_class,
            )
        )

    return ReflectedClass(
        file_path=path,
        class_name=class_name,
        super_name=super_name,
        class_flags=infer_class_flags(class_name, super_name),
        properties=properties,
    )


def write_generated_header(cls: ReflectedClass) -> None:
    macro_name = f"{cls.class_name.upper()}_GENERATED_BODY"
    generated_h = f"""#pragma once

class {cls.class_name};

template<>
struct TIsUClassReflected<{cls.class_name}>
{{
    static constexpr bool Value = true;
}};

#define {macro_name}() \\
public: \\
    using ThisClass = {cls.class_name}; \\
    using Super = {cls.super_name}; \\
    static UClass* StaticClass(); \\
    UClass* GetClass() const override {{ return StaticClass(); }}
"""

    generated_path = GENERATED_DIR / f"{cls.class_name}.generated.h"
    generated_path.write_text(generated_h, encoding="utf-8")

def build_generated_cpp(classes: list[ReflectedClass]) -> str:
    cpp_lines = [
        "// This file is generated. Do not edit manually.",
        "",
    ]

    for cls in classes:
        rel_include = os.path.relpath(cls.file_path, GENERATED_DIR).replace("\\", "/")
        cpp_lines.append(f'#include "{rel_include}"')

    cpp_lines += [
        "",
        '#include "Object/Class.h"',
        '#include "Object/ObjectFactory.h"',
        '#include "Object/ReflectionRegistry.h"',
        "",
    ]

    for cls in classes:
        cpp_lines += [
            f"UClass* {cls.class_name}::StaticClass()",
            "{",
            "    static UClass Class(",
            f'        "{cls.class_name}",',
            "        Super::StaticClass(),",
            f"        sizeof({cls.class_name}),",
            f"        {cls.class_flags},",
            "        []() -> UObject*",
            "        {",
            f"            return UObjectManager::Get().CreateObject<{cls.class_name}>();",
            "        });",
            "",
            "    static bool bRegistered = false;",
            "    if (!bRegistered)",
            "    {",
            "        bRegistered = true;",
            "",
        ]

        for prop in cls.properties:
            object_class_arg = (
                f"{prop.object_class}::StaticClass()"
                if prop.object_class
                else "nullptr"
            )
            cpp_lines += [
                f'        Class.AddProperty({{ "{prop.name}",',
                f"                            {prop.reflected_type},",
                f"                            offsetof({cls.class_name}, {prop.name}),",
                f"                            sizeof({prop.cpp_type}),",
                f"                            {prop.flags},",
                f"                            {object_class_arg} }});",
                "",
            ]

        cpp_lines += [
            "        FReflectionRegistry::Get().RegisterUClass(&Class);",
            "    }",
            "",
            "    return &Class;",
            "}",
            "",
            "namespace",
            "{",
            f"    struct FAutoRegister{cls.class_name}",
            "    {",
            f"        FAutoRegister{cls.class_name}()",
            "        {",
            f"            {cls.class_name}::StaticClass();",
            "        }",
            "    };",
            "",
            f"    static FAutoRegister{cls.class_name} GAutoRegister{cls.class_name};",
            "}",
            "",
        ]

    return "\n".join(cpp_lines)


def main() -> int:
    classes: list[ReflectedClass] = []
    class_names: dict[str, Path] = {}

    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    for file_path in sorted(PROJECT_DIR.rglob("*.h")):
        if should_skip(file_path):
            continue

        reflected_class = parse_header(file_path)
        if reflected_class:
            existing_path = class_names.get(reflected_class.class_name)
            if existing_path:
                raise ReflectionGeneratorError(
                    f"{file_path.relative_to(PROJECT_DIR)}: duplicate UCLASS name "
                    f"'{reflected_class.class_name}' already declared in "
                    f"{existing_path.relative_to(PROJECT_DIR)}"
                )
            class_names[reflected_class.class_name] = file_path
            classes.append(reflected_class)
            write_generated_header(reflected_class)

    GENERATED_CPP.write_text(build_generated_cpp(classes), encoding="utf-8")

    print(f"[GenerateReflection] Generated {len(classes)} reflected classes")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ReflectionGeneratorError as exc:
        print(f"[GenerateReflection] error: {exc}", file=sys.stderr)
        sys.exit(1)
