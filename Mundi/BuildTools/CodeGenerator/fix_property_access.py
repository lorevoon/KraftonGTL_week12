#!/usr/bin/env python3
"""
UPROPERTY가 protected/private에 있으면 public으로 이동시키는 스크립트
"""

import re
from pathlib import Path
from typing import List, Tuple


def find_uproperty_lines(content: str) -> List[Tuple[int, str]]:
    """UPROPERTY가 있는 라인들 찾기"""
    lines = content.split('\n')
    uproperty_lines = []

    for i, line in enumerate(lines):
        if 'UPROPERTY' in line:
            uproperty_lines.append((i, line))

    return uproperty_lines


def get_access_specifier_at_line(lines: List[str], line_num: int) -> str:
    """특정 라인의 접근 제어자 찾기 (역순 탐색)"""
    current_access = 'private'  # class의 기본값

    for i in range(line_num, -1, -1):
        line = lines[i].strip()
        if line.startswith('public:'):
            return 'public'
        elif line.startswith('protected:'):
            return 'protected'
        elif line.startswith('private:'):
            return 'private'
        elif line.startswith('class ') or line.startswith('struct '):
            # struct는 기본 public
            if line.startswith('struct '):
                return 'public'
            break

    return current_access


def extract_property_block(lines: List[str], start_line: int) -> Tuple[int, int]:
    """
    UPROPERTY 매크로부터 시작해서 변수 선언까지 포함한 블록 추출
    Returns: (start_line, end_line)
    """
    # UPROPERTY(...) 매크로가 여러 줄일 수 있음
    i = start_line

    # UPROPERTY 매크로 끝 찾기
    while i < len(lines):
        if ')' in lines[i] and 'UPROPERTY' in ''.join(lines[start_line:i+1]):
            break
        i += 1

    # 다음 줄이 실제 변수 선언
    i += 1
    while i < len(lines):
        line = lines[i].strip()
        # 변수 선언 라인 (세미콜론으로 끝나거나 중괄호 초기화)
        if line and (';' in line or '=' in line):
            return (start_line, i)
        # 빈 줄이나 주석은 스킵
        if not line or line.startswith('//'):
            i += 1
            continue
        return (start_line, i)

    return (start_line, start_line)


def move_uproperties_to_public(file_path: Path) -> bool:
    """파일의 UPROPERTY들을 public으로 이동"""
    content = file_path.read_text(encoding='utf-8')
    lines = content.split('\n')

    # UPROPERTY 찾기
    uproperty_info = []
    for i, line in enumerate(lines):
        if 'UPROPERTY' in line and not line.strip().startswith('//'):
            access = get_access_specifier_at_line(lines, i)
            if access != 'public':
                start, end = extract_property_block(lines, i)
                uproperty_info.append({
                    'start': start,
                    'end': end,
                    'access': access,
                    'lines': lines[start:end+1]
                })

    if not uproperty_info:
        return False

    print(f"  Found {len(uproperty_info)} non-public UPROPERTY blocks")

    # 역순으로 제거 (인덱스 유지)
    for prop in reversed(uproperty_info):
        # 제거
        del lines[prop['start']:prop['end']+1]

    # public: 섹션 찾기 (GENERATED_REFLECTION_BODY 이후)
    generated_line = -1
    for i, line in enumerate(lines):
        if 'GENERATED_REFLECTION_BODY()' in line:
            generated_line = i
            break

    if generated_line == -1:
        print(f"  [WARNING] No GENERATED_REFLECTION_BODY found, skipping {file_path.name}")
        return False

    # GENERATED_REFLECTION_BODY 이후 첫 public: 찾기
    public_line = -1
    for i in range(generated_line, len(lines)):
        if lines[i].strip().startswith('public:'):
            public_line = i
            break

    if public_line == -1:
        # public: 없으면 GENERATED_REFLECTION_BODY 다음에 추가
        lines.insert(generated_line + 1, '')
        lines.insert(generated_line + 2, 'public:')
        public_line = generated_line + 2

    # public: 다음에 UPROPERTY 블록들 삽입
    insert_pos = public_line + 1

    # 빈 줄 추가
    lines.insert(insert_pos, '')
    lines.insert(insert_pos + 1, '    // ===== Lua-Bindable Properties (Auto-moved from protected/private) =====')
    insert_pos += 2

    # 각 UPROPERTY 블록 삽입
    for prop in uproperty_info:
        lines.insert(insert_pos, '')
        for prop_line in prop['lines']:
            lines.insert(insert_pos + 1, prop_line)
            insert_pos += 1
        insert_pos += 1

    # 파일 쓰기
    new_content = '\n'.join(lines)
    file_path.write_text(new_content, encoding='utf-8')

    return True


def main():
    """메인 함수"""
    print("=" * 60)
    print(" UPROPERTY Access Modifier Fixer")
    print("=" * 60)

    # 컴포넌트 디렉토리
    components_dir = Path("Mundi/Source/Runtime/Engine/Components")
    actors_dir = Path("Mundi/Source/Runtime/Engine/GameFramework")
    core_dir = Path("Mundi/Source/Runtime/Core/Object")

    fixed_files = []

    for directory in [components_dir, actors_dir, core_dir]:
        if not directory.exists():
            print(f"[WARNING] Directory not found: {directory}")
            continue

        print(f"\nScanning: {directory}")

        for header_file in directory.glob("*.h"):
            # generated 파일은 스킵
            if '.generated.' in header_file.name:
                continue

            print(f"  Checking: {header_file.name}")

            try:
                if move_uproperties_to_public(header_file):
                    fixed_files.append(header_file)
                    print(f"  [SUCCESS] Fixed: {header_file.name}")
            except Exception as e:
                print(f"  [ERROR] Error processing {header_file.name}: {e}")

    print("\n" + "=" * 60)
    print(f" Complete! Fixed {len(fixed_files)} file(s)")
    print("=" * 60)

    if fixed_files:
        print("\nFixed files:")
        for f in fixed_files:
            print(f"  - {f}")


if __name__ == '__main__':
    main()
