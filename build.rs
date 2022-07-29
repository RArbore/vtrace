/*
 * This file is part of vtrace.
 * vtrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace. If not, see <https://www.gnu.org/licenses/>.
 */

use std::env;
use std::path::Path;
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let profile = env::var("PROFILE").unwrap().to_uppercase();

    Command::new("cc")
        .args(&[
            "-march=native",
            "-static",
            if profile == "RELEASE" { "-O3" } else { "-g" },
            format!("-D{}", profile).as_str(),
            "-c",
            "src/render.c",
            "-o",
            format!("{}/render.o", out_dir).as_str(),
        ])
        .status()
        .unwrap();

    Command::new("ar")
        .args(&["rcs", "librender.a", "render.o"])
        .current_dir(&Path::new(&out_dir))
        .status()
        .unwrap();

    Command::new("glslc")
        .args(&["shaders/test.vert", "-o", "shaders/test.vert.spv"])
        .status()
        .unwrap();

    Command::new("glslc")
        .args(&["shaders/test.frag", "-o", "shaders/test.frag.spv"])
        .status()
        .unwrap();

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=render");
    println!("cargo:rustc-link-lib=dylib=glfw");
    println!("cargo:rustc-link-lib=dylib=vulkan");
    println!("cargo:rerun-if-changed=src/render.c");
    println!("cargo:rerun-if-changed=src/render.h");
    println!("cargo:rerun-if-changed=shaders/test.vert");
    println!("cargo:rerun-if-changed=shaders/test.frag");
}
