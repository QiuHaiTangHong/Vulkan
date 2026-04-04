import json
import os
import platform
import subprocess
import shutil
import sys
import re
from pathlib import Path
from typing import TypedDict, Dict, List, Optional, Union

class DependencyInfo(TypedDict):
    url: str
    tag: str
    cmake_args: List[str]
    only_copy: Optional[bool]
    skip_build: Optional[bool]
    parent_command: Optional[List[str]]
    files: Optional[List[str]]

class Config(TypedDict):
    dependencies: Dict[str, DependencyInfo]
    install_root_path: str
    generator: Optional[str]
    build_type: Optional[str]
    global_cmake_args: List[str]

def filter_platform_args(args: List[str]) -> List[str]:
    """
    不仅过滤以 $ 开头的参数，还处理参数内部的 $Tag: 格式。
    例如: "-DCMAKE_C_FLAGS=-w $Win:-DWIN32" 
    在 Windows 下变为 "-DCMAKE_C_FLAGS=-w -DWIN32"
    在 Linux 下变为 "-DCMAKE_C_FLAGS=-w"
    """
    current_system = platform.system().lower()
    tags = {
        "windows": "$Win:",
        "linux": "$Linux:"
    }
    current_tag = tags.get(current_system, "")
    all_tags = list(tags.values())
    filtered: List[str] = []
    for arg in args:
        processed_arg = arg
        for tag in all_tags:
            if tag == current_tag:
                processed_arg = re.sub(re.escape(tag), "", processed_arg, flags=re.IGNORECASE)
            else:
                pattern = re.escape(tag) + r"[^\s]*"
                processed_arg = re.sub(pattern, "", processed_arg, flags=re.IGNORECASE)
        processed_arg = re.sub(r'\s+', ' ', processed_arg).strip()
        if processed_arg:
            filtered.append(processed_arg)
    return filtered

def merge_cmake_args(global_args: List[str], local_args: List[str]) -> List[str]:
    """
    合并参数，局部参数覆盖全局参数。
    识别格式: -D<KEY>=<VALUE> 或 -D<KEY>
    """
    merged_map: dict[str, str] = {}
    def add_to_map(args_list: List[str]):
        for arg in args_list:
            if arg.startswith("-D"):
                kv_part = arg[2:]
                key = kv_part.split('=')[0]
                merged_map[key] = arg
            else:
                merged_map[arg] = arg

    add_to_map(global_args)
    add_to_map(local_args) # 局部覆盖全局
    return list(merged_map.values())

def run_command(cmd: List[str], cwd: Optional[Union[str, Path]] = None) -> None:
    """
    执行系统命令
    cmd: 字符串列表形式的命令
    cwd: 命令执行的路径
    """
    print(f">>> 正在路径 [{cwd if cwd else '当前目录'}] 执行命令: {' '.join(cmd)}")
    subprocess.check_call(cmd, cwd=cwd)

def sync() -> None:
    script_path: Path = Path(__file__).parent.absolute()
    project_root_path: Path = script_path
    os.chdir(project_root_path)

    with open("dependencies.json", "r", encoding="utf-8") as f:
        config: Config = json.load(f)
    
    system_name: str = platform.system().lower()
    generator: str = config.get("generator") or "Ninja"
    build_type: str = config.get("build_type") or "Release"
    raw_global_args: List[str] = config.get("global_cmake_args", [])
    global_cmake_args = [arg.replace("${build_type}", build_type) for arg in raw_global_args]
    global_filtered_cmake_args = filter_platform_args(global_cmake_args)

    install_root_path: Path = Path(config["install_root_path"].replace("${project_root}", str(project_root_path))) / system_name
    install_root_path.mkdir(parents=True, exist_ok=True)

    deps_root_path: Path = project_root_path / "deps"
    deps_root_path.mkdir(parents=True, exist_ok=True)
    

    print(f">>> 项目根路径: {project_root_path}\n")
    print(f">>> 安装根路径: {install_root_path}\n")
    print(f">>> 依赖下载根路径: {deps_root_path}\n")

    for name, info in config['dependencies'].items():
        print(f"--- 处理依赖: {name} ---")
        current_install_path = install_root_path / name
        if current_install_path.exists():
            print(f"{name} 已存在，跳过安装。")
            continue
        current_install_path.mkdir(parents=True, exist_ok=True)
        current_src_path: Path = deps_root_path / name
        
        if not current_src_path.exists():
            clone_cmd: List[str] = [
                "git", "clone", "--depth", "1", 
                "--branch", info['tag'],
                info['url'], str(current_src_path)
            ]
            run_command(clone_cmd)
        else:
            print(f"{name} 源码已存在，跳过克隆。")

        if "parent_command" in info and info["parent_command"]:
            for cmd_tpl in info["parent_command"]:
                actual_cmd = cmd_tpl.replace("${current_src_path}", str(current_src_path))
                cmd_list = [sys.executable, actual_cmd] if actual_cmd.endswith(".py") else [actual_cmd]
                run_command(cmd_list, cwd=current_src_path)

        # 构建与安装逻辑
        if info.get('only_copy'):
            print(f">>> {name}: 执行文件拷贝...")
            if current_install_path.exists(): shutil.rmtree(current_install_path)
            current_install_path.mkdir(parents=True)
            files_to_copy = info.get("files")
            if files_to_copy:
                for f_name in files_to_copy:
                    src_file = current_src_path / f_name
                    dest_file = current_install_path / f_name
                    if src_file.exists():
                        dest_file.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(src_file, dest_file)
                        print(f">>> 已拷贝: {f_name}")
                    else:
                        print(f">>> 警告: 找不到文件 {src_file}")
            else:
                shutil.copytree(current_src_path, current_install_path, dirs_exist_ok=True)
        else:
            build_dir: Path = current_src_path / "build"
            build_dir.mkdir(exist_ok=True)
            
            cmake_configure: List[str] = [
                "cmake",
                "-G", generator,
                "-S", str(current_src_path),
                "-B", str(build_dir),
                f"-DCMAKE_INSTALL_PREFIX={current_install_path}"
            ]
            
            local_filtered_cmake_args = filter_platform_args(info.get('cmake_args', []))
            final_args = merge_cmake_args(global_filtered_cmake_args, local_filtered_cmake_args)
            cmake_configure.extend(final_args)

            run_command(cmake_configure)

            if info.get("skip_build"):
                print(f">>> {name} 是头文件库，跳过编译直接执行 CMake Install...")
                run_command(["cmake", "--install", str(build_dir)], cwd=current_src_path)
            else:
                print(f">>> {name} 执行完整构建与安装...")
                run_command(["cmake", "--build", str(build_dir), "--config", build_type], cwd=current_src_path)
                run_command(["cmake", "--install", str(build_dir), "--config", build_type], cwd=current_src_path)

    print("\n所有依赖处理完成！")

if __name__ == "__main__":
    sync()