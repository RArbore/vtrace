use vulkano::device::physical::*;
use vulkano::instance::*;

fn main() {
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("ERROR: Couldn't create Vulkan instance.");
    let physical = PhysicalDevice::enumerate(&instance)
        .next()
        .expect("ERROR: No physical device is available.");
}
