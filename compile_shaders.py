#!/usr/bin/env python3

import shutil
import subprocess
import sys
from pathlib import Path
from subprocess import CalledProcessError


def main():
    if len(sys.argv) < 2:
        print("provide input and output directory!")
        sys.exit(1)

    if shutil.which("glslc") is None:
        print("Error! glslc was not found in your PATH! Make sure it's installed and available in the PATH.")
        print("You can download glslc here:  https://github.com/google/shaderc/#downloads")
        sys.exit(1)

    in_dir = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)

    glsl_files = list(in_dir.glob("**/*.glsl"))
    if not glsl_files:
        print(f"Warning! no shader files found in {in_dir}")
        sys.exit(0)

    for input_file in glsl_files:
        relative_path = input_file.relative_to(in_dir)
        output_file = (out_dir / relative_path).with_suffix(".spv")
        output_file.parent.mkdir(parents=True, exist_ok=True)

        if ".vert" in input_file.stem:
            shader_type = "vertex"
        elif ".frag" in input_file.stem:
            shader_type = "fragment"
        elif ".comp" in input_file.stem:
            shader_type = "compute"
        elif ".lib" in input_file.stem:
            continue
        else:
            print(f"Skipping {input_file} (unknown type)")
            continue

        try:
            subprocess.run([
                "glslc",
                f"-fshader-stage={shader_type}",
                str(input_file),
                "-o",
                str(output_file),
            ], check=True)
            print(f"Compiled {input_file} -> {output_file}")
        except CalledProcessError:
            print(f"Error compiling shader at {input_file}")
            sys.exit(1)


if __name__ == "__main__":
    main()
