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

#[cfg(target_os = "windows")]
use ash::extensions::khr::Win32Surface;
#[cfg(all(unix, not(target_os = "android"), not(target_os = "macos")))]
use ash::extensions::khr::XlibSurface;
#[cfg(target_os = "macos")]
use ash::extensions::mvk::MacOSSurface;
use ash::*;

use ash::extensions::ext::DebugUtils;
use ash::extensions::khr::Surface;

use std::ffi::CString;
use std::ptr;

pub struct VulkanManager {
    _entry: Entry,
    instance: Instance,
}

impl VulkanManager {
    unsafe fn create_instance(entry: &Entry) -> Instance {
        let app_name = CString::new("vtrace-rs").unwrap();
        let engine_name = CString::new("Custom").unwrap();

        let app_info = vk::ApplicationInfo {
            s_type: vk::StructureType::APPLICATION_INFO,
            p_next: ptr::null(),
            p_application_name: app_name.as_ptr(),
            application_version: 1 << 2,
            p_engine_name: engine_name.as_ptr(),
            engine_version: 1 << 22,
            api_version: vk::API_VERSION_1_3,
        };

        let extension_names = required_extension_names();

        let create_info = vk::InstanceCreateInfo {
            s_type: vk::StructureType::INSTANCE_CREATE_INFO,
            p_next: ptr::null(),
            flags: vk::InstanceCreateFlags::empty(),
            p_application_info: &app_info,
            pp_enabled_layer_names: ptr::null(),
            enabled_layer_count: 0,
            pp_enabled_extension_names: extension_names.as_ptr(),
            enabled_extension_count: extension_names.len() as u32,
        };

        entry
            .create_instance(&create_info, None)
            .expect("ERROR: Failed to create instance")
    }

    pub unsafe fn new() -> Self {
        let entry = Entry::load().unwrap();
        let instance = Self::create_instance(&entry);

        VulkanManager {
            _entry: entry,
            instance,
        }
    }
}

impl Drop for VulkanManager {
    fn drop(&mut self) {
        unsafe {
            self.instance.destroy_instance(None);
        }
    }
}

#[cfg(target_os = "macos")]
fn required_extension_names() -> Vec<*const i8> {
    vec![
        Surface::name().as_ptr(),
        MacOSSurface::name().as_ptr(),
        DebugUtils::name().as_ptr(),
    ]
}

#[cfg(all(windows))]
fn required_extension_names() -> Vec<*const i8> {
    vec![
        Surface::name().as_ptr(),
        Win32Surface::name().as_ptr(),
        DebugUtils::name().as_ptr(),
    ]
}

#[cfg(all(unix, not(target_os = "android"), not(target_os = "macos")))]
fn required_extension_names() -> Vec<*const i8> {
    vec![
        Surface::name().as_ptr(),
        XlibSurface::name().as_ptr(),
        DebugUtils::name().as_ptr(),
    ]
}
