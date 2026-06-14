#!/usr/bin/env python3
"""批量清理代码注释脚本"""

import re
import sys
from pathlib import Path


def clean_file_header(content: str) -> str:
    """去掉文件头部的 /** ... */ 注释块（包含@author, @version等）"""
    # 匹配文件开头的 /** ... */ 块
    pattern = r'^/\*\*\s*\n.*?@file.*?\*/\s*\n'
    content = re.sub(pattern, '', content, flags=re.MULTILINE | re.DOTALL)
    return content


def clean_lab_comments(content: str) -> str:
    """去掉 lab 相关的教学注释"""
    # 去掉 Welcome to tinycoro lab 的大段注释（更强的匹配）
    pattern = r'/\*\*\s*\n.*?Welcome to tinycoro lab.*?\*/'
    content = re.sub(pattern, '', content, flags=re.MULTILINE | re.DOTALL)

    # 去掉其他多行教学注释（包含 lab 关键字的）
    pattern = r'/\*\*\s*\n.*?\blab\d+\b.*?\*/'
    content = re.sub(pattern, '', content, flags=re.MULTILINE | re.DOTALL | re.IGNORECASE)

    # 去掉 TODO[labX] 相关注释
    content = re.sub(r'\s*//\s*TODO\[lab\w+\]:.*\n', '\n', content)

    # 去掉 [[CORO_TEST_USED(labX)]] 标记
    content = re.sub(r'\[\[CORO_TEST_USED\(lab\w+\)\]\]\s*', '', content)

    return content


def clean_verbose_comments(content: str) -> str:
    """去掉过于详细的实现说明注释"""
    # 去掉简单的英文描述性多行注释
    pattern = r'/\*\*\s*\n\s*\*\s*@brief\s+.*?\n\s*\*.*?\n\s*\*/\s*\n'
    content = re.sub(pattern, '', content, flags=re.MULTILINE | re.DOTALL)

    lines = content.split('\n')
    cleaned_lines = []

    for i, line in enumerate(lines):
        # 跳过过长的单行注释（超过100字符且全是注释）
        if re.match(r'^\s*//\s*.{80,}$', line):
            continue

        # 跳过明显的实现细节注释（中文）
        if re.search(r'//.*\b(创建时的调度逻辑|实现逻辑|具体步骤|执行流程|什么都不做)\b', line):
            continue

        # 跳过箭头符号的详细说明
        if re.search(r'//.*(->|→).*\b(必须|应该|需要)\b', line):
            continue

        cleaned_lines.append(line)

    return '\n'.join(cleaned_lines)


def translate_common_comments(content: str) -> str:
    """将常见的英文注释翻译成中文"""
    translations = {
        r'//\s*lock\s+the\s+mutex': '// 加锁',
        r'//\s*unlock\s+the\s+mutex': '// 解锁',
        r'//\s*wait\s+for': '// 等待',
        r'//\s*notify': '// 通知',
        r'//\s*send': '// 发送',
        r'//\s*receive': '// 接收',
        r'//\s*closed': '// 已关闭',
        r'//\s*TODO:': '// 待办:',
        r'//\s*FIXME:': '// 修复:',
        r'//\s*NOTE:': '// 注意:',
    }

    for en_pattern, zh_replacement in translations.items():
        content = re.sub(en_pattern, zh_replacement, content, flags=re.IGNORECASE)

    return content


def clean_code_file(file_path: Path) -> bool:
    """清理单个代码文件的注释"""
    try:
        # 尝试多种编码
        content = None
        for encoding in ['utf-8', 'gbk', 'gb2312', 'latin-1']:
            try:
                with open(file_path, 'r', encoding=encoding) as f:
                    content = f.read()
                break
            except UnicodeDecodeError:
                continue

        if content is None:
            print(f"无法读取文件（编码问题）: {file_path}", file=sys.stderr)
            return False

        original_content = content

        # 依次应用各种清理规则
        content = clean_file_header(content)
        content = clean_lab_comments(content)
        content = clean_verbose_comments(content)
        content = translate_common_comments(content)

        # 去掉多余的空行（3个以上连续空行压缩为2个）
        content = re.sub(r'\n{4,}', '\n\n\n', content)

        # 如果内容有变化，写回文件
        if content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return True

        return False

    except Exception as e:
        print(f"Error processing {file_path}: {e}", file=sys.stderr)
        return False


def main():
    """主函数"""
    project_root = Path(__file__).parent.parent

    # 查找所有需要处理的文件
    patterns = ['**/*.hpp', '**/*.cpp']
    exclude_dirs = {'third_party', 'build', '.git', '.claude'}

    files_to_process = []
    for pattern in patterns:
        for file_path in project_root.glob(pattern):
            # 排除第三方库和构建目录
            if any(excluded in file_path.parts for excluded in exclude_dirs):
                continue
            files_to_process.append(file_path)

    print(f"找到 {len(files_to_process)} 个文件需要处理")

    modified_count = 0
    for file_path in files_to_process:
        if clean_code_file(file_path):
            modified_count += 1
            print(f"✓ {file_path.relative_to(project_root)}")

    print(f"\n完成! 共修改了 {modified_count} 个文件")


if __name__ == '__main__':
    main()
