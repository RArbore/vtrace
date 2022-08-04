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
use std::process::exit;
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let profile = env::var("PROFILE").unwrap().to_uppercase();

    let c_files = [
        "entry.c",
        "descriptor.c",
        "device.c",
        "swapchain.c",
        "pipeline.c",
        "command.c",
        "memory.c",
        "sync.c",
    ];

    let o_files = [
        "entry.o",
        "descriptor.o",
        "device.o",
        "swapchain.o",
        "pipeline.o",
        "command.o",
        "memory.o",
        "sync.o",
    ];

    for (c_file, o_file) in std::iter::zip(c_files, o_files) {
        let status = Command::new("cc")
            .args(&[
                "-march=native",
                "-static",
                if profile == "RELEASE" { "-O3" } else { "-g" },
                format!("-D{}", profile).as_str(),
                "-c",
                format!("lib/{}", c_file).as_str(),
                "-o",
                format!("{}/{}", out_dir, o_file).as_str(),
            ])
            .status()
            .unwrap();
        if !status.success() {
            exit(1);
        }

        println!("cargo:rerun-if-changed=lib/{}", c_file);
    }

    let status = Command::new("ar")
        .args(std::iter::once("rcs").chain(std::iter::once("librender.a").chain(o_files)))
        .current_dir(&Path::new(&out_dir))
        .status()
        .unwrap();
    if !status.success() {
        exit(1);
    }

    let status = Command::new("glslc")
        .args(&["shaders/test.vert", "-o", "shaders/test.vert.spv"])
        .status()
        .unwrap();
    if !status.success() {
        exit(1);
    }

    let status = Command::new("glslc")
        .args(&["shaders/test.frag", "-o", "shaders/test.frag.spv"])
        .status()
        .unwrap();
    if !status.success() {
        exit(1);
    }

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=render");
    println!("cargo:rustc-link-lib=dylib=glfw");
    println!("cargo:rustc-link-lib=dylib=vulkan");
    println!("cargo:rerun-if-changed=lib/common.h");
    println!("cargo:rerun-if-changed=test.vert");
    println!("cargo:rerun-if-changed=test.frag");
}
