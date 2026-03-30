import json
import os
import platform
import subprocess
import shutil
from pathlib import Path
from typing import TypedDict, Dict, List, Optional, Union

class DependencyInfo(TypedDict):
    url: str
    tag: str
    cmake_args: List[str]
    only_copy: Optional[bool]
    skip_build: Optional[bool]

class Config(TypedDict):
    dependencies: Dict[str, DependencyInfo]
    install_root_path: str
    build_type: Optional[str]

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
    build_config: str = config.get("build_type") or "Release"

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

        # 构建与安装逻辑
        if info.get('only_copy'):
            print(f">>> {name} 拷贝库文件跳过构建安装...\n")
            
            if current_install_path.exists(): shutil.rmtree(current_install_path)
            shutil.copytree(current_src_path, current_install_path)
        else:
            build_dir: Path = current_src_path / "build"
            build_dir.mkdir(exist_ok=True)
            
            cmake_configure: List[str] = [
                "cmake", "-S", str(current_src_path), "-B", str(build_dir),
                f"-DCMAKE_INSTALL_PREFIX={current_install_path}",
                f"-DCMAKE_BUILD_TYPE={build_config}"
            ]
            
            cmake_configure.extend(info.get('cmake_args', []))
            
            if platform.system() == "Windows":
                cmake_configure.append("-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL")

            run_command(cmake_configure)

            if info.get("skip_build"):
                print(f">>> {name} 是头文件库，跳过编译直接执行 CMake Install...")
                run_command(["cmake", "--install", str(build_dir)], cwd=current_src_path)
            else:
                print(f">>> {name} 执行完整构建与安装...")
                run_command(["cmake", "--build", str(build_dir), "--config", build_config], cwd=current_src_path)
                run_command(["cmake", "--install", str(build_dir), "--config", build_config], cwd=current_src_path)

    print("\n所有依赖处理完成！")

if __name__ == "__main__":
    sync()