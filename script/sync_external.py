#!/usr/bin/env python3
import argparse
import datetime as dt
import glob
import logging
import os
import shutil
import subprocess
import traceback
import sys
import hashlib
from urllib.parse import urlparse
from concurrent.futures import ThreadPoolExecutor
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional
import yaml
from pydantic import BaseModel
from rich.console import Console
from rich.live import Live
from rich.table import Table

console = Console()
logger = logging.getLogger(__name__)


class ProjectConfig(BaseModel):
    name: str
    url: Optional[str] = None
    tag: Optional[str] = None
    branch: Optional[str] = None
    commit: Optional[str] = None
    use_symlink: Optional[bool] = None
    strip_components: Optional[int] = 1
    sha256: Optional[str] = None

    class Config:
        extra = "forbid"  # do NOT allow extra fields


class UpdateStatus(str, Enum):
    SUCCESS = "success"
    SKIPPED = "skipped"
    ERROR = "error"


def run_command(command: str, cwd: Optional[str] = None) -> str:
    process = subprocess.Popen(
        command,
        shell=True,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    logger.info(
        "Running command: [%s] cwd=[%s] PID:[%d]",
        command,
        cwd if cwd else os.getcwd(),
        process.pid,
    )

    stdout, stderr = process.communicate()
    returncode = process.wait()
    if returncode == 0:
        ret = stdout.strip()
        logger.info("Command:[%s],returns:[%s]PID:[%d]", command, ret, process.pid)
        return ret
    else:
        logger.error(
            "Command:[%s],failed with return value:[%d],error:[%s]PID:[%d]",
            command,
            returncode,
            stderr.strip(),
            process.pid,
        )
        raise subprocess.CalledProcessError(
            returncode, command, output=stdout, stderr=stderr
        )


def is_git_repo(path: str) -> bool:
    git_path = os.path.join(path, ".git")
    has_git_dir = os.path.isdir(git_path)
    has_git_file = False
    if not has_git_dir and os.path.isfile(git_path):
        # .git can be a text file pointing to the actual gitdir
        try:
            with open(git_path, "r", encoding="utf-8") as f:
                header = f.read(256)
            has_git_file = header.lstrip().lower().startswith("gitdir:")
        except Exception:
            has_git_file = False

    if not (has_git_dir or has_git_file):
        return False

    try:
        out = run_command("git rev-parse --is-inside-work-tree", cwd=path).strip()
        return out.lower() == "true"
    except subprocess.CalledProcessError:
        return False


def get_short_commit_id(repo_path: str) -> str:
    try:
        return run_command("git rev-parse --short HEAD", cwd=repo_path).strip()
    except subprocess.CalledProcessError:
        return "HEAD"


def compute_file_hash(path: str) -> str:
    sha = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            sha.update(chunk)
    return sha.hexdigest()


def update_repo(
    repo_path: str, branch: Optional[str] = None, tag: Optional[str] = None
) -> UpdateStatus:
    if tag is None:
        branch = branch or "main"  # Default to "main" if branch/tag is None

    try:
        status = run_command("git status --porcelain", cwd=repo_path)
        if status:  # If there's any output, there are uncommitted changes
            console.print(
                f"[red]Skipping update of {repo_path} - working directory has uncommitted changes"
            )
            logger.warning(
                " Skipping update of %s - working directory has uncommitted changes",
                repo_path,
            )
            return UpdateStatus.SKIPPED
        if branch:
            logging.info("Updating %s with branch %s ...", repo_path, branch)
            run_command(f"git fetch origin {branch} --depth 1", cwd=repo_path)
            run_command(f"git checkout -B {branch} FETCH_HEAD", cwd=repo_path)
        else:
            logging.info("Updating %s with tag %s ...", repo_path, tag)
            run_command(
                f"git fetch origin tag {tag} --no-tags --depth 1", cwd=repo_path
            )
            run_command(f"git checkout tags/{tag}", cwd=repo_path)
        return UpdateStatus.SUCCESS
    except subprocess.CalledProcessError as e:
        logging.error(
            "Error updating %s with branch %s: %s\nTraceback: %s",
            repo_path,
            branch,
            str(e),
            traceback.format_exc(),
        )
        console.print(
            f"[red]Error updating {repo_path} with branch {branch}: {str(e)}[/red]\n"
        )
        return UpdateStatus.ERROR


def checkout_commit(branch: str, commit: str, project_dest: str) -> None:
    current_head = run_command("git rev-parse HEAD", cwd=project_dest).strip()
    if not current_head.startswith(commit):
        try:
            # Get more commit history instead of top one
            run_command(f"git fetch origin {branch} --depth 50", cwd=project_dest)
            # Define backup branch name
            backup_branch = (
                f"{branch}_{current_head[:8]}"  # Shorten hash for readability
            )

            # Check if backup branch already exists
            existing_branches = (
                run_command("git branch --list", cwd=project_dest).strip().splitlines()
            )
            # Clean branch names (remove leading * ' or ')
            existing_branches = [
                b.strip().replace("* ", "").strip() for b in existing_branches
            ]
            if backup_branch not in existing_branches:
                # Create new branch only if it doesn't exist
                run_command(f"git branch {backup_branch}", cwd=project_dest)
                console.print(
                    f"[yellow]Created back branch '{backup_branch}' in {project_dest}[/yellow]"
                )
            else:
                console.print(
                    f"[yellow]Backup branch '{backup_branch}' already exists in {project_dest}, skipping creation[/yellow]"
                )
            # Switch to the target branch and then the commit
            run_command(f"git checkout {branch}", cwd=project_dest)
            run_command(f"git checkout {commit}", cwd=project_dest)

        except subprocess.CalledProcessError as e:
            # Handle case where the commit is invalid
            console.print(
                f"[red]Invalid commit {commit} in {project_dest}: {e.stderr.strip()}[/red]"
            )
            raise e


def process_local_project(
    dest_dir: str, project: ProjectConfig, statuses: Dict[str, List[str]], live: Live
) -> None:
    name = project.name
    # Reuse `url' as local directory path
    source_dir = project.url or ""
    statuses[name][0] = "local_dir"
    statuses[name][1] = "N/A"
    statuses[name][2] = "Processing local_dir..."
    live.update(render_table(statuses))

    try:
        # Resolve absolute source path
        if not os.path.isabs(source_dir):
            source_dir = os.path.abspath(os.path.join(os.getcwd(), source_dir))
        if not os.path.exists(source_dir):
            raise FileNotFoundError(f"Local directory not found: {source_dir}")

        project_dest = os.path.join(dest_dir, name)

        # Ensure parent exists and cleanup previous dest if any
        parent = os.path.dirname(project_dest)
        os.makedirs(parent, exist_ok=True)
        if os.path.lexists(project_dest):  # includes existing broken symlink
            if os.path.islink(project_dest) or os.path.isfile(project_dest):
                os.unlink(project_dest)
            else:
                shutil.rmtree(project_dest)

        # Resolve strategy: use use_symlink; default True
        use_symlink = project.use_symlink if project.use_symlink is not None else True

        if use_symlink:
            # Create absolute symlink for robustness
            logger.info(
                "Local dir strategy: symlink (use_symlink=True). src=%s dest=%s",
                source_dir,
                project_dest,
            )
            console.print(
                f"[yellow]Linking {project_dest} -> {source_dir} (symlink)[/yellow]"
            )
            os.symlink(source_dir, project_dest, target_is_directory=True)
            statuses[name][2] = "[green]√ Linked[/green]"
        else:
            # Prefer rsync for speed and robustness; fallback to Python copy
            logger.info(
                "Local dir strategy: copy (use_symlink=False). src=%s dest=%s",
                source_dir,
                project_dest,
            )
            console.print(
                f"[yellow]Copying local_dir {source_dir} -> {project_dest}[/yellow]"
            )

            if has_rsync():
                logger.info("rsync detected on PATH, using rsync for copy.")
                statuses[name][2] = "Copying via rsync..."
                live.update(render_table(statuses))
                rsync_copy_dir(
                    source_dir,
                    project_dest,
                    statuses=statuses,
                    project_name=name,
                    live=live,
                )
            else:
                logger.info("rsync not found, falling back to Python copytree.")
                copytree_with_progress(
                    source_dir,
                    project_dest,
                    statuses=statuses,
                    project_name=name,
                    live=live,
                )
            statuses[name][2] = "[green]V Copied[/green]"
    except Exception as e:
        statuses[name][2] = f"[red]X Error: {e} [/red]"
    finally:
        live.update(render_table(statuses))


def _sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_join(base: str, *paths: str) -> str:
    abs_base = os.path.abspath(base)
    candidate = os.path.abspath(os.path.normpath(os.path.join(abs_base, *paths)))
    if not (candidate == abs_base or candidate.startswith(abs_base + os.sep)):
        raise Exception(f"Blocked path traversal: {candidate}")
    return candidate


def clone_project(
    dest_dir: str,
    project: ProjectConfig,
    statuses: Dict[str, List[str]],
    live: Live,
) -> None:
    name: str = project.name
    url: Optional[str] = project.url
    tag: Optional[str] = project.tag
    branch: Optional[str] = project.branch
    branch_name: str = branch or tag or "main"
    commit: Optional[str] = project.commit

    statuses[name][0] = branch_name
    statuses[name][1] = commit if commit else "HEAD"
    # live.update(render_table(statuses))
    project_dest: str = os.path.join(dest_dir, name)
    skip_clone_project: bool = False
    logger.info(
        "Clone project=%s, url=%s, tag=%s, branch=%s, commit=%s",
        name,
        url,
        tag,
        branch_name,
        commit,
    )

    # Handle local directory or archive (local/remote) before git
    if url:
        abs_url_path = url
        if not os.path.isabs(abs_url_path):
            abs_url_path = os.path.abspath(os.path.join(os.getcwd(), abs_url_path))
        if os.path.isdir(abs_url_path):
            logger.info(
                "Detected local directory url -> processing as local_dir: %s",
                abs_url_path,
            )
            process_local_project(dest_dir, project, statuses, live)
            return
        # Archive detection (local or remote)
        parsed = urlparse(url)
        path_for_ext = (
            parsed.path if parsed.scheme in {"http", "https"} else abs_url_path
        )
        lower = path_for_ext.lower()
        if (
            lower.endswith(".tar.gz")
            or lower.endswith(".tgz")
            or lower.endswith(".tar.xz")
            or lower.endswith(".txz")
            or lower.endswith(".zip")
        ):
            logger.info("Detected archive url -> processing as archive: %s", url)
            raise RuntimeError("not support yet")

    try:
        if os.path.exists(project_dest):
            logger.info("Project dest dir %s exists already", project_dest)
            skip_clone_project = True
            if is_git_repo(project_dest):
                logger.info(
                    "Project dest dir %s contains a git repo already, try to update it ...",
                    project_dest,
                )
                cleanup = False
                statuses[name][2] = "Updating..."

                update_status = update_repo(project_dest, branch, tag)
                if update_status == UpdateStatus.SUCCESS:
                    statuses[name][2] = "[green]V Updated![green]"
                elif update_status == UpdateStatus.SKIPPED:
                    statuses[name][2] = "[yellow]Skipped (dirty)[/yellow]"
                    return
                elif update_status == UpdateStatus.ERROR:
                    cleanup = True
            else:
                logger.info(
                    "Project dest dir %s is not a git repo, try to remove it ...",
                    project_dest,
                )
                cleanup = True

            if cleanup:
                timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
                backup_path = f"{project_dest}_{timestamp}.bak"
                shutil.move(project_dest, backup_path)
                console.print(
                    f"[yellow]Already moved {project_dest} to {backup_path} as backup[/yellow]"
                )
                skip_clone_project = False

        if not skip_clone_project:
            statuses[name][2] = "cloning ..."
            run_command(
                f"git clone --depth 1 --branch {branch_name} {url} {project_dest}"
            )

        if commit:
            checkout_commit(branch_name, commit, project_dest)

        # Get the actual short commit id after successful clone/update
        statuses[name][1] = get_short_commit_id(project_dest)
        statuses[name][2] = "[green] successfully![green]"
    except (subprocess.CalledProcessError, OSError) as e:
        statuses[name][2] = f"[red]x Error: {e}[/red]"
    finally:
        live.update(render_table(statuses))


def render_table(statuses: Dict[str, List[str]]) -> Table:
    table = Table(title="Projects List")
    table.add_column("Project", justify="left")
    table.add_column("Tag", justify="left")
    table.add_column("Commit", justify="left")
    table.add_column("Status", justify="left")
    for name, values in statuses.items():
        tag, commit, status = values[0], values[1], values[2]
        commit = commit if commit else ""
        table.add_row(name, tag, commit, status)
    return table


def copy(source_path: str, dest_path: str, *, force: bool = True) -> None:
    is_directory = os.path.isdir(source_path)
    dest_path = os.path.normpath(dest_path)
    parent_dir = os.path.dirname(os.path.abspath(dest_path))
    os.makedirs(parent_dir, exist_ok=True)

    if force and os.path.exists(dest_path):
        if os.path.isdir(dest_path):
            shutil.rmtree(dest_path)
        else:
            os.unlink(dest_path)
    console.print(f"[yellow]Copying {source_path} -> {dest_path}[/yellow]")
    if is_directory:
        shutil.copytree(source_path, dest_path)
    else:
        shutil.copy2(source_path, dest_path)


def copytree_with_progress(
    source_dir: str,
    dest_dir: str,
    *,
    statuses: Optional[Dict[str, List[str]]] = None,
    project_name: Optional[str] = None,
    live: Optional[Live] = None,
    force: bool = True,
) -> None:
    if force and os.path.exists(dest_dir):
        shutil.rmtree(dest_dir)
    os.makedirs(dest_dir, exist_ok=True)

    # First pass: count files for progress
    total_files = 0
    for root, dirs, files in os.walk(source_dir):
        # Skip Vcs directories
        dirs[:] = [d for d in dirs if d not in {".git", ".hg", ".svn"}]
        total_files += len(files)

    copied = 0
    last_update = 0

    for root, dirs, files in os.walk(source_dir):
        # Skip Vcs directories
        dirs[:] = [d for d in dirs if d not in {".git", ".hg", ".svn"}]
        rel_root = os.path.relpath(root, source_dir)
        dest_root = dest_dir if rel_root == "." else os.path.join(dest_dir, rel_root)
        os.makedirs(dest_root, exist_ok=True)

        for f in files:
            src = os.path.join(root, f)
            dst = os.path.join(dest_root, f)
            shutil.copy2(src, dst)
            copied += 1
            # Update status intermittently (about every 0.1s or on final)
            if statuses is not None and project_name is not None and live is not None:
                # Avoid too frequent updates
                import time

                now = time.time()
                if (now - last_update) > 0.1 or copied == total_files:
                    percent = (copied / total_files * 100) if total_files else 100
                    statuses[project_name][
                        2
                    ] = f"Copying... {copied}/{total_files} files ({percent:.1f}%"
                    live.update(render_table(statuses))
                    last_update = now


def has_rsync() -> bool:
    from shutil import which

    return which("rsync") is not None


def rsync_copy_dir(
    source_dir: str,
    dest_dir: str,
    *,
    statuses: Optional[Dict[str, List[str]]] = None,
    project_name: Optional[str] = None,
    live: Optional[Live] = None,
) -> None:
    # Ensure trailing slash semantics for rsync directory contents copy
    src = os.path.join(source_dir, "")
    dst = os.path.join(dest_dir, "")
    os.makedirs(dest_dir, exist_ok=True)

    excludes = [".git", ".hg", ".svn"]
    exclude_args = " ".join([f"--exclude='{e}'" for e in excludes])
    cmd = (
        f"rsync -a --delete --mkpath --human-readable --info=progress2 "
        f"{exclude_args} '{src}' '{dst}'"
    )

    def on_stdout(line: str) -> None:
        if statuses is not None and project_name is not None and live is not None:
            statuses[project_name][2] = f"rsync: {line.strip()}"
            live.update(render_table(statuses))

    run_command_stream(cmd, on_stdout_line=on_stdout)


def run_command_stream(
    command: str,
    *,
    cwd: Optional[str] = None,
    on_stdout_line: Optional[callable] = None,
    on_stderr_line: Optional[callable] = None,
) -> int:
    process = subprocess.Popen(
        command,
        shell=True,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )
    logger.info(
        "Streaming command: [%s] cwd=[%s] PID:[%d]",
        command,
        cwd or os.getcwd(),
        process.pid,
    )
    # Read stdout
    if process.stdout is not None:
        for line in process.stdout:
            if on_stdout_line:
                on_stdout_line(line)
    # Read stderr
    if process.stderr is not None:
        for line in process.stderr:
            if on_stderr_line:
                on_stderr_line(line)

    returncode = process.wait()
    if returncode != 0:
        logger.error("Command failed: [%s] rc=%d", command, returncode)
        raise subprocess.CalledProcessError(returncode, command)
    return returncode


def link(source_path: str, link_path: str, *, force: bool = True) -> None:
    is_directory = os.path.isdir(source_path)
    link_path = os.path.normpath(link_path)
    parent_dir: str = os.path.dirname(os.path.abspath(link_path))
    os.makedirs(parent_dir, exist_ok=True)
    # Ensure the parent directory exists (called twice intentionally in original code)
    os.makedirs(parent_dir, exist_ok=True)

    depth: int = link_path.count(os.sep)  # Count directory separators in link_path
    if depth > 0:
        relative_prefix: str = os.sep.join(
            [".."] * depth
        )  # Generate correct number of "../"
        relative_source_path: str = os.path.normpath(
            os.path.join(relative_prefix, source_path)
        )
    else:
        relative_source_path = (
            source_path  # No modification needed for single-level paths
        )

    if force and os.path.exists(link_path):
        if os.path.islink(link_path) or os.path.isfile(link_path):
            os.unlink(link_path)  # Remove file or symlink
        elif os.path.isdir(link_path):
            shutil.rmtree(link_path)  # Remove directory

    if not os.path.islink(link_path) or os.readlink(link_path) != relative_source_path:
        console.print(
            f"[yellow]Creating symbolic link: {link_path} -> {relative_source_path}[/yellow]"
        )
        os.symlink(relative_source_path, link_path, target_is_directory=is_directory)


def load_config(config_file: str) -> dict:
    with open(config_file, encoding="utf-8") as f:
        config_content = f.read()
        logger.info("Configuration content:\n%s", config_content)
        config = yaml.safe_load(config_content)

    raw_projects = config.get("projects", [])
    if raw_projects is None:
        raw_projects = []
    if not isinstance(raw_projects, list):
        raise ValueError("'projects' must be a list in the configuration")

    projects: List[ProjectConfig] = [ProjectConfig(**p) for p in raw_projects]
    config["projects"] = projects
    return config


def clone_or_update_projects(
    dest_dir: str,
    projects: List[ProjectConfig],
    statuses: Dict[str, List[str]],
    live: Live,
    max_workers: int,
) -> None:
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = [
            executor.submit(clone_project, dest_dir, project, statuses, live)
            for project in projects
        ]
        for future in futures:
            future.result()


def cleanup_unused_dirs(dest_dir: str, configured_projects: set) -> None:
    existing_dirs = set(os.listdir(dest_dir))
    # Exclude directories ending with '.bak' (backups)
    unused_dirs = {
        d for d in existing_dirs - configured_projects if not d.endswith(".bak")
    }
    for unused in unused_dirs:
        unused_path = os.path.join(dest_dir, unused)
        # If it's a symlink, just unlink the link (do not rmtree)
        if os.path.islink(unused_path):
            console.print(f"[yellow]Removing unused symlink: {unused}[/yellow]")
            os.unlink(unused_path)
        elif os.path.isdir(unused_path):
            console.print(f"[yellow]Removing unused directory: {unused}[/yellow]")
            shutil.rmtree(unused_path)


def cleanup_old_backups(dest_dir: str, max_backups: int = 5) -> None:
    backups = [f for f in os.listdir(dest_dir) if f.endswith(".bak")]
    backups.sort()
    if len(backups) > max_backups:
        for old_backup in backups[:-max_backups]:
            backup_path = os.path.join(dest_dir, old_backup)
            console.print(f"[yellow]Deleted old backup dir: {backup_path}[/yellow]")
            shutil.rmtree(backup_path)


def setup_logging(log_dir: str) -> str:
    timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = Path(log_dir) / f"sync_externals_{timestamp}.log"

    class LongMessageFormatter(logging.Formatter):
        MESSAGE_WIDTH = 120
        TIME_STAMP_LENGTH = 23
        LEVEL_LENGTH = 7
        PID_LENGTH = 5
        TID_LENGTH = 10
        NEW_LINE_PADDING = " " * (
            MESSAGE_WIDTH + TIME_STAMP_LENGTH + TIME_STAMP_LENGTH + LEVEL_LENGTH + 3
        )

        def format(self, record):
            timestamp = self.formatTime(record, self.datefmt)
            levelname = record.levelname.ljust(
                self.LEVEL_LENGTH
            )
            pid = str(record.process).rjust(
                self.PID_LENGTH
            )
            tid = str(record.thread).rjust(
                self.TID_LENGTH
            )

            message = record.getMessage()
            file_line = f"@{record.pathname}:{record.lineno}"

            if len(message) <= self.MESSAGE_WIDTH:
                padded_message = message.ljust(self.MESSAGE_WIDTH)
                return (
                    f"{timestamp} {levelname} {pid}:{tid} {padded_message} {file_line}"
                )

            lines = []
            lines.append(f"{timestamp} {levelname} {pid}:{tid} {message}")
            lines.append(f"{self.NEW_LINE_PADDING}{file_line}")
            return "\n".join(lines)


    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s,%(levelname)s,%(message)s @%(pathname)s:%(lineno)d",
        datefmt="%Y-%m-%d %H:%M:%S.%f",
        handlers=[
            logging.FileHandler(log_file),
        ],
    )
    for handler in logging.getLogger().handlers:
        handler.setFormatter(LongMessageFormatter())

    max_number_of_logs: int = 3
    log_files = sorted(glob.glob(f"{log_dir}/sync_externals_*.log"), reverse=True)
    if len(log_files) > max_number_of_logs:
        for old_log in log_files[max_number_of_logs:]:
            try:
                os.remove(old_log)
                logger.info("Removed old log file: %s", old_log)
            except OSError as e:
                logger.warning("Failed to remove %s error: %s", old_log, e)
    return str(log_file)


def main() -> None:
    parser = argparse.ArgumentParser(description="Clone and manage Git externals.")
    parser.add_argument(
        "--config",
        default="externals.yml",
        help="Path to config file (default: %(default)s)",
    )
    parser.add_argument(
        "--dest",
        default=".externals",
        help="Destination directory (default: %(default)s)",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=1,
        help="Number of worker threads (default: %(default)s)",
    )
    args = parser.parse_args()
    config_file: str = args.config
    dest_dir: str = args.dest
    max_workers: int = args.threads
    
    os.makedirs(dest_dir, exist_ok=True)
    logfile = setup_logging(dest_dir)
    logger.info("Start to sync external repos")

    projects: List[ProjectConfig] = []
    try:
        config = load_config(config_file)
        projects = config.get("projects", [])
        for project in projects:
            if project.tag and project.branch:
                raise ValueError(
                    f"Both tag and branch are specified for prject '{project.name}"
                )
    except Exception as e:
        console.print(f"[red]Configuration error: {e}[/red]")
        return

    statuses: Dict[str, List[str]] = {
        project.name: ["", "", "Waiting..."] for project in projects
    }

    with Live(render_table(statuses), refresh_per_second=1, console=console) as live:
        clone_or_update_projects(dest_dir, projects, statuses, live, max_workers)

    configured_projects = {project.name for project in projects}
    cleanup_unused_dirs(dest_dir, configured_projects)
    cleanup_old_backups(dest_dir)

    errors = [name for name, stat in statuses.items() if "X" in stat[2]]
    if errors:
        console.print("\n[red]Errors occurred in the following projects:[/red]")
        for error in errors:
            console.print(f"[red]- {error} [/red]")
    else:
        console.print("\n[green]All projects cloned successfully![/green]")

    # Log the final status table
    final_table = render_table(statuses)
    log_console = Console(
        file=open(logfile, "a", encoding="utf-8")
    )  # Append to log file
    with log_console.capture() as capture:
        log_console.print(final_table)

    logger.info("Final status:\n%s", capture.get())
    console.print(f"\n[green]Log file: {logfile}[/green]")


if __name__ == "__main__":
    main()
