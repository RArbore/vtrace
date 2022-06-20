/*
 * This file is part of vtrace-rs.
 * vtrace-rs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace-rs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace-rs. If not, see <https://www.gnu.org/licenses/>.
 */

use std::fs;
use std::process::Command;

const SHADERS: &[(&str, &str)] = &[("compute", "comp"), ("vert", "vert"), ("frag", "frag")];

fn main() {
    for (shader, stage) in SHADERS {
        Command::new("rm")
            .args(&[String::from("-f"), format!("shaders/{}.spv", shader)])
            .status()
            .unwrap();
        let glslc_output = Command::new("glslc")
            .args(&[
                format!("-fshader-stage={}", stage),
                format!("shaders/{}.hlsl", shader),
                String::from("-o"),
                format!("shaders/{}.spv", shader),
            ])
            .output()
            .unwrap();

        if !glslc_output.status.success() {
            let error = String::from_utf8(glslc_output.stderr).unwrap();
            let filename = format!("glslc_error_{}.txt", shader);
            println!("cargo:warning=GLSLC ERROR. See {} for details.", filename);
            fs::write(filename, error).unwrap();
        }

        println!("cargo:rustc-if-changed=shaders/{}.hlsl", shader);
        println!("cargo:rustc-if-changed=shaders/{}.spv", shader);
    }
}
